//=============================================================================
// SkSpreadSheet CellClass 
//=============================================================================
#ifndef SkCellClass_hpp
#define SkCellClass_hpp

#include "../include/SkCell.hpp"

namespace SkSpreadSheet {

    class tCellModelClass : public tModelClass {
    private:
        tSharedString m_Family;
    public:
        /// @brief      Constructor tCellModelClass
        /// @param[in]  sName tString
        /// @param[in]  sLabel tString
        /// @param[in]  sFamily tString
        /// @param[in]  sFunctionCreate tFunctionCreate
        tCellModelClass(tString sName, tString sLabel,tString sFamily, tFunctionCreate sFunctionCreate);
    
        /// @brief      Return true to save model in JSON (See tVariant).
        /// @return     tBool
        tBool SaveModel() override;
    
        /// @brief      Return true to save data in JSON (See tVariant).
        /// @return     tBool
        tBool SaveData() override;
    
        /// @brief      Set family
        /// @param[in]  sFamily  tString
        void   Family(tString sFamily);
        
        /// @brief      Get Family
        /// @return     tString
        tString Family();

        /// @brief Writer Json (includes fm for cell-class models).
        void Json(Writer<StringBuffer>* sWriter) override;
    };

    //! tCellClass ===================================================================
    class  tCellClass : public tVariantClass {
    public:
        /// @brief      Constructor tCellClass
        tCellClass();
        
        /// @brief      Constructor tCellClass with Value
        /// @param[in]  sValue tVariant
        tCellClass(const tVariant& sValue);
        
        /// @brief      Copy Constructor
        /// @param[in]  sCellClass tCellClass&
        tCellClass(const tCellClass& sCellClass);

        /// @brief      Destructor tCellClass
        virtual ~tCellClass();
        
        /// @brief      Clone (used by tVariant)
        /// @return     tVirtualClass*
        tVirtualClass* Clone() override;
     
        /// @brief      Return name of class (for factory)
        /// @return     tString
        tString ClassName() const override;

        /// @brief     Cover other cell (see JsonView)
        /// @return    tBool
        virtual tBool Cover() const;
        
        /// @brief      Is calculation propagation (true by default, false for tCellClassAttribute)
        /// @return     tBool
        tBool IsCalculationPropagation() const override;

        /// @brief      Write JSON.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Read JSON.
        /// @param[in]  sValue Value&
        void Json(const rapidjson::Value& sValue) override;

#ifdef checksp
        /// @brief      Check.
        virtual void Check();
#endif
#ifdef _DEBUGSK		
        /// @brief      Debug.
        tString Debug() override;
#endif
    };

}; // end of namespace 

#endif
