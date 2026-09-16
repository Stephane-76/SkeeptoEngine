//=============================================================================
// SkInterfaceCompil.hpp
//=============================================================================

#ifndef SkInterfaceCompil_hpp
#define SkInterfaceCompil_hpp

#include "SkApplication.hpp"
#include "SkItem.hpp"
using namespace SkRoot;



namespace SkSpreadSheet {
    // Forward declarations to avoid heavy includes
    class tFormula;
    class tSheet;
    class tItem;

    /// @brief      Interface compil
    /// used for lemon interface
    class tInterfaceCompil   {
    public:
        /// @brief      Destructor
        virtual ~tInterfaceCompil() = default;

        /// @brief      Clear  Ref and formula
        virtual void ClearVectorRefAndDeleteDependant()=0;

        // Formula
        /// @brief      Set formula
        /// @param[in]  sFormula tFormula*
        virtual void Formula(const tFormula* sFormula)=0;

        /// @brief      Return formula
        /// @return     tFormula*
        virtual tFormula* Formula()=0;
        
        /// @brief      Return sheet
        /// @return     tSheet*
        virtual tSheet* Sheet()=0;

        /// @brief      Push reference
        /// @param[in]  sItem tItem*
        /// @param[in]  sDependent tBool
        virtual void PushRef(tItem* sItem,tBool sDependent=true)=0;
        
        /// @brief Return Vector Item
        /// @return tVectorItem*
        virtual tVectorItem* VectorItem()=0;
        
        /// @brief      Return row index
        /// @return     tIndex
        virtual tIndex RowIndex()=0;

        /// @brief      Return col index
        /// @return     tIndex
        virtual tIndex ColIndex()=0;
    };
}

#endif // SkInterfaceCompil_hpp
