// SkUndoRedoJson.hpp
#include "../include/SkColRowCellRange.hpp"

namespace SkSpreadSheet {

class tUndoRedoJsonCallBack {
private:
    /// @brief        Sheet.
    tSheet* m_Sheet;
    /// @brief        Json Writer.
    Writer<StringBuffer>* m_Writer;
public:
    /// @brief        Constructor.
    tUndoRedoJsonCallBack();
    /// @brief        Destructor.
    ~tUndoRedoJsonCallBack();

    /// @brief        Call back for cell in tIndex coordinate.
    /// @param[in]    sPoint tTempoPoint*
    /// @return        tBool
    tBool CallBackCell(tTempoPoint* sPoint);

    /// @brief        Call back for cell in tIndex coordinate.
    /// @param[in]    sRect tTempoRect*
    /// @return        tBool
    tBool CallBackRange(tTempoRect* sRect);

    /// @brief        Write Cell in Json
    /// @param[in]    sRef tString
    /// @param[in]    sSheet tSheet*
    /// @return       tString
    tString WriteCell(tString sRef, tSheet* sSheet);

    /// @brief        Read operation from striing.
    /// @param[in]    sJson tString
    /// @return       tBool
    tBool ReadCell(tString sJson);
    
    /// @brief        Write Range Named.
    /// @param[in]    sRef tString
    /// @param[in]    sSheet tSheet*
    /// @return       tString
    tString WriteRange(tString sRef, tSheet* sSheet);
    
    
    /// @brief        Write Range Named.
    /// @param[in]    sJson tString
    /// @return       tBool
    tBool ReadRange(tString sJson);
   
};
}
    
