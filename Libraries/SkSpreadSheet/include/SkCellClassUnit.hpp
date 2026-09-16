//=============================================================================
// SkSpreadSheet CellClassUnit  
//=============================================================================
#ifndef SkCellClassUnit_hpp
#define SkCellClassUnit_hpp

#include <SkUnit.hpp>
#include "SkCell.hpp"
#include "SkCellClass.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

  
    class tCellModelClassUnit : public tCellModelClass {
    public:
        /// @brief      Constructor tCellModelClass
        /// @param[in]  sName tString
        /// @param[in]  sLabel  tString
        /// @param[in]  sFunctionCreate tFunctionCreate
        tCellModelClassUnit(tString sName,tString sLabel, tFunctionCreate sFunctionCreate);
    
        /// @brief      return true for place model in Json (See tVariant).
        /// @return     tBool
        tBool SaveModel() override;
    
        /// @brief      return true for place datal in Json (See tVariant).
        /// @return     tBool
        tBool SaveData() override;
    };

    //! tCellClass ===================================================================
    class  tCellClassUnit : public tCellClass {
    private:
        tClassUnit  m_UnitClass;
        tClassUnit  m_UnitClassFrac;
        tBool       m_Error;
        
        /// @brief      Return Left Position of tClassUnit
        /// @param[out] sLeft tClassUnit*&
        /// @param[out] sLeftFrac tClassUnit*&
        /// @param[out] sRight tClassUnit*&
        /// @param[out] sRightFrac tClassUnit*&
        void MultiplyOrDivide(tClassUnit* sLeft,tClassUnit* sLeftFrac,tClassUnit* sRight,tClassUnit* sRightFrac);
     public:
        /// @brief      Constructor tCellClass
        tCellClassUnit();
        
        /// @brief      Constructor tCellClass with Value
        tCellClassUnit(const tVariant& sValue,const tClassUnit& sClassUnit);

        /// @brief      Constructor with numerator and denominator units (e.g. m/s).
        tCellClassUnit(const tVariant& sValue, const tClassUnit& sNumerator, const tClassUnit& sDenominator);
        
        /// @brief      Copy Constructor
        /// @param[in]  sCellClass tCellClass&
        tCellClassUnit(const tCellClassUnit& sCellClassUnit);

        /// @brief      Destructor tCellClass
        virtual ~tCellClassUnit();
        
        /// @brief      Clone (used by tVariant)
        /// @return     tVirtualClass
        tVirtualClass* Clone() override;
       
        /// @brief      return Name of class (for factory)
        /// @return     tString
        tString ClassName() const override;
        
        /// @brief      Writer Json.
        /// @param[in]  sWriter Writer<StringBuffer>*
        /// @param[in]  sValue tBool  R1C1 for paste
        /// @param[in]  sDiff tPoint* sDiff (for copy diff)
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      Reader Json.
        /// @param[in]  sValue Value&
        void Json(const rapidjson::Value& sValue) override;
        
        /// @brief      Is Generate Json for Javascript
        /// @return     tBool
        tBool IsJsonJavaScript() override;
        
        // React =============================================================
        /// @brief      Writer Json for Javascript.
        /// @param[in]  sWriter Writer<StringBuffer>*
        void JsonJavaScript(Writer<StringBuffer>* sWriter) override;
        
        /// @brief      IsSameFamily
        /// @param[in]  sClassUnit& tClassUnit
        /// @return     tBool
        tBool IsSameFamily(tClassUnit& sClassUnit);
        
        /// @brief      + operator
        /// @return     tVariant
        tVariant Operator_plus(tBool sLeft, const tVariant& sVariant) override;
        
        /// @brief      - operator
        /// @return     tVariant
        tVariant Operator_minus(tBool sLeft, const tVariant& sVariant) override;
        
        /// @brief      * operator
        /// @return     tVariant
        tVariant Operator_multiply(tBool sLeft, const tVariant& sVariant) override;
        
        /// @brief      / operator 
        /// @return     tVariant
        tVariant Operator_divide(tBool sLeft, const tVariant& sVariant) override;
        
#ifdef checksp
        /// @brief      Check.
        void Check() override;
#endif
        
        /// @brief      Debug.
        /// @return     tString
        tString Str();
        
#ifdef _DEBUGSK
        /// @brief      Debug.
        /// @return     tString
        tString Debug() override;
#endif
    };

    /// @brief      Register CellClassUnit.
    /// @return     tBool
    tBool RegisterCellClassUnit();

    /// @brief      Register CellClassUnit.
    /// @return     tBool
    tBool UnRegisterCellClassUnit();

}; // end of namespace 

#endif // SkCellClassUnit_hpp
