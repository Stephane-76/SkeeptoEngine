//=============================================================================
// SkSpreadSheet CellClassAttribute
//=============================================================================
#ifndef SkCellClassAttribute_hpp
#define SkCellClassAttribute_hpp

#include  <SkModelClass.hpp>
#include "SkCellAttribute.hpp"
#include "SkCell.hpp"
#include "SkCellClass.hpp"

namespace SkSpreadSheet {
	//! tCellModelClass =======================================================
    class tCellModelClassAttribute : public  tCellModelClass { //tModelObj {
		public:
            /// @brief      Constructor tCellModelClass
            /// @param[in]  sName tString
            /// @param[in]  sLabel tString
            /// @param[in]  sFunctionCreate tFunctionCreate
            /// @return        tString
			tCellModelClassAttribute(tString sName,tString sLabel,tString sFamily, tFunctionCreate sFunctionCreate);
        
            /// @brief      return true for place model in Json (See tVariant).
            /// @return     tBool
            tBool SaveModel() override;
            
            /// @brief      return true for place datal in Json (See tVariant).
            /// @return     tBool
            tBool SaveData() override;
            
            /// @brief      SetProperty of  attribute by instance
            /// @param[in]  sThis tVirtualClass
            /// @param[in]  sName tString
            /// @param[in]  sValue Variant
            void Property(tVirtualClass* sThis, tString sName, tVariant sValue) override;
            
            /// @brief      GetProperty of  attribute by instance
            /// @param[in]  sThis tVirtualClass
            /// @param[in]  sName tString
            /// @return        tVariant
			tVariant Property(tVirtualClass* sThis, tString sName) override;
	};

	//! tAttributeElem      ===================================================
	class tAttributeElem : public tClass {
	private:
        ///! Name of Attribute
		tSharedString	m_Name;
        //! Allocator ref of attribute see   tAllocatorCellAttribute;
		tAllocatorRef	m_AllocatorRef;
	public:
        /// @brief      Constructor tAttributeElem
        /// @param[in]  sName tString
        /// @param[in]  sAllocatorRef tAllocatorRef
		tAttributeElem(tString sName, tAllocatorRef sAllocatorRef);
        
        /// @brief      Constructor  copy tAttributeElem
        /// @param[in]  sAttributeElem tAttributeElem&
        tAttributeElem(const tAttributeElem& sAttributeElem);
        
        /// @brief      Get Attribute Name
        /// @return     tString;
		tString Name();

        /// @brief      Get Attribute Allocator ref
        /// @return     tAllocatorrRef
		tAllocatorRef AllocatorRef();
	};
	typedef vector<tAttributeElem> tVectorAttributeElem;

	//! tAttributeContainer ===================================================
	class tAttributeContainer : public tClass {
	private:
        //!   Atrribute Vector
		tVectorAttributeElem   m_VectorAttributeElem;
        //! One instance for One Class (see tCellClass(const tCellClass& sCellClassAttribute);
        //! When we copy tCellclass we only keep one instance of tAttribute container
		tInt			 	   m_NbInstance;
    
        /// @brief      Alloc Attribute
        /// @param[in]  sName tString
        /// @param[in]  sCellRoot tCell*
        /// @return     tAllocatorrRef
        SkInline tAllocatorRef Alloc(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange);
        
        /// @brief      Insert Attribute
        /// @param[in]  sName tString
        /// @param[in]  sCellRoot tCell*
        /// @return     tAllocatorrRef
        SkInline tAllocatorRef Insert(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange);
	public:
        /// @brief      Constructor tAttributeContainer
		tAttributeContainer();

        /// @brief      Destructor r tAttributeContainer
        ~tAttributeContainer();
        
        /// @brief      Inc NbInstance
		void IncInstance();
        
        /// @brief      Dec NbInstance
		void DecInstance();
        
        /// @brief      return NbInstance
        /// @return     tInt
		tInt NbInstance();

        /// @brief      return AllocatorRef of Attribute Instance
        /// @return     tAlocatorRef
		tAllocatorRef Find(tString sName);

        /// @brief      Delete Attribute by name
        /// @return     tBool
        tBool DeleteCellAttribute(tString sName, tColRowCellRange* sColRowCellRange);
		
        /// @brief      Return the attribute or create it
        /// @param[in]  sName tString
        /// @param[in]  sCellRoot tCell*
        /// @param[in]  sColRowCellRange tColRowCellRange*
        /// @return     tCellAttribute*
        tCellAttribute* CellAttributeGetorCreate(tString sName, tCell* sCellRoot, tColRowCellRange* sColRowCellRange);
        
        /// @brief      return CellAttribute by Pas
        /// @param[in]  sPos tSize
        /// @param[in]  sCellRoot tCell*
        /// @param[in]  sColRowCellRange tColRowCellRange*
        /// @return     tCellAttribute*
        tCellAttribute* CellAttribute(tSize sPos, tColRowCellRange* sColRowCellRange);

        /// @brief      Clear all CellAttribute
        /// @param[in]  sColRowCellRange tColRowCellRange*
        void ClearAttribute(tColRowCellRange* wColRowCellRange);

        /// @brief      Clear all Formula & Value of CellAttribute
        /// @param[in]  sColRowCellRange tColRowCellRange*
		void ClearFormulaVariant(tColRowCellRange* wColRowCellRange);

        /// @brief      Place the attribute on the root
        /// @param[in]  tCell* sCell
        /// @param[in]  sColRowCellRange tColRowCellRange*
        void Rooted(tCell* sCell, tColRowCellRange* sColRowCellRange);
        
        /// @brief      Return size
        /// @return     tSize
        tSize Size();
#ifdef checksp
		/// @brief      Check.
		void Check(tColRowCellRange* sColRowCellRange);
#endif
#ifdef _DEBUGSK
		/// @brief      Debug.
        /// @return tString
		tString Debug(tColRowCellRange* sColRowCellRange);
#endif
	};

	//! tCellClassAttribute ======================================================
    class  tCellClassAttribute : public tCellClass {
	private:
        ///!  ClassName
        tSharedString         m_ClassName;
        //! RefName for calcul
        tSharedString         m_RefName;
       
		//! Allocator indice for SkTableAllocatorTable (Sheet) 
		tIndex			      m_SheetIndice;
        //! Root Cell Ref (owner cell)
		tAllocatorRef		  m_CellRootRef;
        //! Container of attribute
		tAttributeContainer*   m_AttributeContainer;
        
        // Id for extern application (React indice of object ==================
        tInt                    m_Id;
        
		//! Model ==================================================================
		tModelClass*         m_ModelClass;
	public:
        /// @brief      Constructor tCellClass
		tCellClassAttribute();
        
        /// @brief      Copy Constructor
        /// @param[in]  sCellClassAttribute tCellClass&
		tCellClassAttribute(const tCellClassAttribute& sCellClassAttribute);

        /// @brief      Destructor tCellClass
        virtual ~tCellClassAttribute();
        
        /// @brief      Clone (used by tVariant)
        /// @return     tVirtualClass
        tVirtualClass* Clone() override;

        /// @brief      Is    Calcul propagation (false for tCellClassAttribute
        /// @return     tBool
        tBool IsCalculationPropagation() const override;

        /// @brief Scalar exposed to formulas (=H6) — backed by tVariantClass::m_Value.
        tVariant& CalculableValue();
        const tVariant& CalculableValue() const;
        void SetCalculableValue(const tVariant& sValue);
        
        /// @brief      Clear all CellAttribute Formula & Value
        void ClearFormulaVariant();

        /// @brief      Clear all CellAttribute 
        void ClearAttribute();
        
        
        //! Pointer of ColRowCellRange
		tColRowCellRange* ColRowCellRange();
        
        /// @brief      return Name of class (for factory)
        /// @return     tString
        virtual tString ClassName() const override;
        
        /// @brief      Set  ClassName
        /// @param[in]  sClassName tString
        virtual void ClassName(tString sClassName);
        
        /// @brief      Set  Name
        /// @param[in]  sName tString
        void RefName(tString sName);
        
        /// @brief      return Name
        /// @return     tString
        tString RefName();

        /// @brief      Set Id
        /// @param[in]  sId tInt
        void Id(tInt sId);
        
        /// @brief      return Id
        /// @return     tInt
        tInt Id();
	
        /// @brief      Set SheetIndice
        /// @param[in]  sSheet Indice tIndex
		void SheetIndice(tIndex sSheetIndice);
        
        /// @brief      return SheetIndice
        /// @return     tIndex
		tIndex SheetIndice();

		/// @brief      Copy col row and indice of ColRowCellRange to attribute.
		/// @param[in]  sCell TCell*
		void Rooted(tCell* sCell);

        /// @brief     Set RootCell
        /// @param[in] sColRowCellRange tColRowCellRange*
        /// @param[in]  sRow tIndex
        /// @param[in]  sCol tIndex
        void CellRootRef(tColRowCellRange* sColRowCellRange,tIndex sRow, tIndex sCol);

        /// @brief     Get root cell
        /// @return    tAlocatorRef
		tAllocatorRef CellRootRef();
        
        
        /// @brief     Set ModelClass
        /// @param[in] sModelClass tModelClass*
        void SetModelClass(tModelClass* sModelClass=nullptr);
        
        /// @brief     Get ModelClass
        /// @return    tModelClass*
        tModelClass* ModelClass();

        /// @brief     Find Cell Attribute
        /// @param[in] sName tString
        /// @return    tCellAttribute*
		tCellAttribute* Find(tString sName);
        
        /// @brief     Delete Cell Attribute
        /// @param[in] sName tString
        /// @return    tBool
        tBool DeleteCellAttribute(tString sName);
        
        /// @brief     Return Cell Attribute ref
        /// @param[in] sName tString
        /// @return    tAllocatorRef
        tAllocatorRef CellAttributeRef(tString sName);

        /// @brief     Return Cell Attribute rby name
        /// @param[in] sName tString
        /// @return    tCellAttribute*
        tCellAttribute* CellAttribute(tString sName);
        
        /// @brief     Return Cell Attribute by postion
        /// @param[in] sPos tIndex
        /// @return    tCellAttribute*
        tCellAttribute* CellAttribute(tIndex sPos);

        /// @brief      Return size
        /// @return     tSize
        tSize	Size();

        /// @brief     Set attribute by vector
        /// @param[in] sVectorCellAttribute tVectorCellAttribute*
        void CellAttribute(tVectorCellAttribute* sVectorCellAttribute);
        
        // Json React ========================================================
        /// @brief      Is    Component React (<div>)
        /// @return     tBool
        tBool IsReactComponent() override;
        
        // Json ===============================================================
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        /// @param[in]    sValue R1C1 for paste
        /// @param{in]    tPoint* sDiff (for copy diff)
        virtual void JsonCell(Writer<StringBuffer>* sWriter,tBool sR1C1 = false, tPoint* sDiff = nullptr);
        
        // @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        /// @param[in]    sValue tBool  R1C1 for paste
        /// @param[in]    sDiff tPoint* sDiff (for copy diff)
        void Json(Writer<StringBuffer>* sWriter) override;
        
        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void Json(const rapidjson::Value& sValue) override;
        
#ifdef checksp
		/// @brief      Check.
        void Check() override;
#endif
#ifdef _DEBUGSK		
		/// @brief      Debug.
		tString Debug() override;
#endif
	};

    tCellClassAttribute* CreateGenericCellClassAttribute();

    /// @brief Register a generic stub model (no properties) if missing from tClassFactory.
    tBool EnsureCellClassModelStub(tString sClassName);

    /// @brief Wire EnsureCellClassModelStub into tClassFactory::Create() fallback.
    void InstallCellClassModelStubHandler();

    /// @brief Register cell-class models from workbook JSON `"models"` array (ReadJson).
    /// @param[in] sModels rapidjson array of model objects (n, l, fm, p)
    /// @return true if at least one model was processed
    tBool RegisterCellClassModelsFromJson(const rapidjson::Value& sModels);

    /// @brief Register stub models for class names referenced in floatingobjects / _$$A cells.
    /// @description Ensures EnsureCellClass / WriteJson work when the "models" section is absent.
    tBool RegisterMissingCellClassModelsFromWorkbookJson(const rapidjson::Value& sWorkbook);

}; // end of namespace 

#endif
