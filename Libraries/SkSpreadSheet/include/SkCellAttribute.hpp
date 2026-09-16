//=============================================================================
// SkSpreadSheet CellAttribute 
//=============================================================================
#ifndef SkCellAttribute_hpp
#define SkCellAttribute_hpp

#include <SkModelClass.hpp>
#include "../include/SkCell.hpp"

namespace SkSpreadSheet {
    //! tCellAttribute ========================================================
    class alignas(SkAlign) tCellAttribute : public tCell {
    private:
        tSharedString			m_Name;
    public:
        /// @brief Constructor
        tCellAttribute();
        
        /// @brief Constructor with name
        /// @param[in] m_Name tString
        tCellAttribute(tString m_Name);

        /// @brief Copy constructor
        /// @param[in] sCellAttribute const tCellAttribute&
        tCellAttribute(const tCellAttribute& sCellAttribute);

        /// @brief Set name
        /// @param[in] sName tString
        void Name(tString sName);
        
        /// @brief Get name
        /// @return tString
        tString Name();

        /// @brief Return reference of cell like A1.coucou
        /// @param[in] sSheetName tBool
        /// @return tString
        const tString StrRef(tBool sSheetName=false) const;
        
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        /// @param[in]  sFormatApi tFormatApi
        /// @param[in]    sValue R1C1 for paste
        /// @param{in]    tPoint* sDiff (for copy diff)
        void Json(Writer<StringBuffer>* sWriter,tBool sR1C1 = false,tPoint* sDiff = nullptr) override;
        
        /// @brief Operator < for sorting
        /// @param[in] sCellAttribute const tCellAttribute&
        /// @return tBool
        tBool operator < (const tCellAttribute& sCellAttribute) const;

#ifdef _DEBUGSK
        /// @brief Debug
        /// @return tString
        tString Debug();
#endif
        
        /// @brief Integrity check (no-op unless compiled with checksp; matches tCell::Check gating).
        void Check();
    };

    typedef vector<tCellAttribute*> tVectorCellAttribute;

}; // End of namespace

#endif
