/*
 * SkConditionalFormat.hpp
 *
 *  Created on: 21 sept. 2025
 *
 */

#ifndef SkConditionalFormat_hpp
#define SkConditionalFormat_hpp

#include "SkApplication.hpp"
#include "SkItem.hpp"
#include "SkFormula.hpp"
#include "SkSharedFormula.hpp"
#include "SkInterfaceCompil.hpp"
#include "SkRange.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

    class tColRowCellRange;

    enum class tConditionalFormatType : tChar {
        t_None,
        t_HighlightCellsRules,
        t_DataBars,
        t_ColorScales,
        t_IconSets,
        t_CustomFormulas
    };

    enum class tIconType : tChar {
        t_None,
        t_Flags,
        t_Arrows,
        t_Shapes,
        t_Indicators,
        t_Ratings
    };

    enum class tPass : tChar {
        t_PassInit,
        t_PassSum,
        t_PassCalculate,
        t_PassApply,
        t_PassFree
    };
    

    //==========================================================================
    class tConditionalRanges : public tClass {
    private:
        tVectorRange            m_VectorRange;
        tConditionalFormatType  m_Type;
    public:
        /// @brief      Constructor
        tConditionalRanges();

        /// @brief      Constructor copy
        tConditionalRanges(const tConditionalRanges& sConditionalKey);

        /// @brief      Destructor
        ~tConditionalRanges();

        /// @brief      Set key (Retun First Range for unicity
        /// @patam[in]     sType ) tConditionalFormatType
        /// @param[in]  sRef  tString
        /// @param[in]  sSheet tSheet*
        void SetKeyAndEnsureRange(tConditionalFormatType sType,tString sRef,tSheet* sSheet);
        
        /// @brief      Return key
        /// @return     tString
        tString Ref() const;
        
        /// @brief      Return key
        /// @return     tString
        tString Key() const;

     
        /// @brief      Return vector reference
        /// @return     tVectorItem*
        tVectorRange* VectorRange();

        /// @brief      Intersect rect
        /// @param[in]  sRect tRect*
        /// @return     tBool
        tBool IntersectRect(tRect* sRect);
        
        /// @brief Get first Cell for compil
        /// @return tCell*
        tCell* FirstCell();

        /// @brief      Remove range
        /// @param[in]  sRange tRange*
        /// @return     tBool
        tBool RemoveRange(tRange* sRange);

        /// @brief      Operator tString
        operator tString() const;
    };

    // =========================================================================
    //! Item Conditionnal Format for each cell 
    class tItemCF : public tClass {
    private:
        tConditionalFormatType  m_Type;
        tFormatRef              m_Css;
        tDouble                 m_Percent;
        tIconType               m_IconType;
        tString                 m_IconString;
        // 0-based position of the selected icon inside the IconSet (0..4 for 3- or
        // 5-icon configurations). Lets the renderer pick a deterministic visual per
        // tier even when the OOXML iconString is not unique (e.g. Indicators "●●●"
        // 3-traffic-light set, where the legacy iconString-only path could not tell
        // low/mid/high apart). Default -1 means "unknown / not an IconSet item".
        tInt                    m_IconIndex;
        // Total number of icons in the parent set (3 or 5). Needed alongside
        // m_IconIndex because Excel ships both 3- and 5-tier variants for the
        // same iconType (e.g. "Arrows" → 3Arrows or 5Arrows) and the renderer
        // picks a different glyph per total. 0 means "unknown".
        tInt                    m_IconTotal;
        
        // DataBars specific parameters
        tString                 m_Color;        // Bar color
        tString                 m_Direction;    // Direction (left-to-right, etc.)
        tString                 m_Style;        // Style (solid, gradient, etc.)
        tDouble                 m_MinValue;     // Minimum value
        tDouble                 m_MaxValue;     // Maximum value
        tString                 m_Param6;       // Additional parameter 6
        tString                 m_Param7;       // Additional parameter 7
    public:
        /// @brief      Default constructor (required so tItemCF can be stored
        ///             by value inside tAllocator, which value-initializes its
        ///             slots with T m_Elem{}).
        tItemCF();

        /// @brief      Constructor
        /// @param[in]  sType tConditionalFormatType
        /// @param[in]  sCss tFormatRef
        /// @param[in]  sPercent tDouble
        /// @param[in]  sIconType tIconType
        /// @param[in]  sIconString tString
        /// @param[in]  sColor tString (for DataBars)
        /// @param[in]  sDirection tString (for DataBars)
        /// @param[in]  sStyle tString (for DataBars)
        /// @param[in]  sMinValue tDouble (for DataBars)
        /// @param[in]  sMaxValue tDouble (for DataBars)
        /// @param[in]  sParam6 tString (for DataBars)
        /// @param[in]  sParam7 tString (for DataBars)
        tItemCF(tConditionalFormatType sType, tFormatRef sCss, tDouble sPercent, tIconType sIconType, tString sIconString, 
                tString sColor = "", tString sDirection = "", tString sStyle = "", tDouble sMinValue = 0.0, tDouble sMaxValue = 100.0, 
                tString sParam6 = "", tString sParam7 = "");

        ///@brief Clear
        void Clear();

        /// @brief      Return cell
        /// @return     tFormatRef
        tFormatRef Css();
                
        /// @brief      Return conditional format type
        /// @return     tConditionalFormatType
        tConditionalFormatType Type();

        /// @brief      Return percent
        /// @return     tDouble
        tDouble Percent();
        
        /// @brief      Return icon type
        /// @return     tIconType
        tIconType IconType();

        /// @brief Return IsonString
        /// @retrun tString
        tString IconString();

        /// @brief      0-based icon index inside the IconSet (-1 if unknown)
        /// @return     tInt
        tInt IconIndex();
        void SetIconIndex(tInt sIndex);

        /// @brief      Total icons in the IconSet (3, 5, or 0 if unknown)
        /// @return     tInt
        tInt IconTotal();
        void SetIconTotal(tInt sTotal);
        
        // DataBars specific getters
        /// @brief      Return bar color
        /// @return     tString
        tString Color();
        
        /// @brief      Return bar direction
        /// @return     tString
        tString Direction();
        
        /// @brief      Return bar style
        /// @return     tString
        tString Style();
        
        /// @brief      Return minimum value
        /// @return     tDouble
        tDouble MinValue();
        
        /// @brief      Return maximum value
        /// @return     tDouble
        tDouble MaxValue();
        
        /// @brief      Return additional parameter 6
        /// @return     tString
        tString Param6();
        
        /// @brief      Return additional parameter 7
        /// @return     tString
        tString Param7();

        /// @brief      Return sort index
        /// @return     tInt
        tInt ReturnSortIndex();
        
        /// @brief      Serialize to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize from JSON
        /// @param[in]  sValue Value&
        void Json(const Value& sValue);
        
        /// @brief      Return JSON string
        /// @return     tString
        tString Json();

        /// @brief      Reset all item fields after allocation.
        void Set(tConditionalFormatType sType, tFormatRef sCss, tDouble sPercent, tIconType sIconType, tString sIconString,
                 tString sColor = "", tString sDirection = "", tString sStyle = "", tDouble sMinValue = 0.0, tDouble sMaxValue = 100.0,
                 tString sParam6 = "", tString sParam7 = "");
        
        // Specialized JSON methods for each conditional format type
        /// @brief      Serialize HighlightCellsRules to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void JsonHighlightCellsRules(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize HighlightCellsRules from JSON
        /// @param[in]  sValue Value&
        void JsonHighlightCellsRules(const Value& sValue);
        
        /// @brief      Serialize DataBars to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void JsonDataBars(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize DataBars from JSON
        /// @param[in]  sValue Value&
        void JsonDataBars(const Value& sValue);
        
        /// @brief      Serialize ColorScales to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void JsonColorScales(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize ColorScales from JSON
        /// @param[in]  sValue Value&
        void JsonColorScales(const Value& sValue);
        
        /// @brief      Serialize IconSets to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void JsonIconSets(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize IconSets from JSON
        /// @param[in]  sValue Value&
        void JsonIconSets(const Value& sValue);
        
    };

    typedef vector<tAllocatorRef> tVectorItemCF;

    class tConditionalFormatContainer;
    // =========================================================================
    //! Conditional Format
    class tConditionalFormat : public tClass, public tInterfaceCompil {
    private:
        //! Container
        tConditionalFormatContainer* m_Container;

        tSheet*                 m_Sheet;
        //! WorkBook
        tWorkBook*              m_WorkBook;
        //! Format Api
        tFormatApi*             m_FormatApi;
        
        //! Ref
        tConditionalRanges       m_ConditionalRanges;
        //! Vector of Select tPoint and tRange
        tVectorItem             m_VectorSelect;
        
        // Conditional Format Type
        tConditionalFormatType  m_Type;
        //! Icon Set Type
        tIconType               m_IconType;
     
        // Custom Formula
        //! Shared formula (formulas with the same content but different references are shared in a single SkFormulaItem)
        tSharedFormula          m_SharedFormula;
    
        //! Vector fo reference Cell or Range in formula
        tVectorItem             m_VectorRef;

        //! formula, condition true and false etc..
        tSharedString                 m_Param1;
        tSharedString                 m_Param2;
        tSharedString                 m_Param3;
        tSharedString                 m_Param4;
        tSharedString                 m_Param5;
        tSharedString                 m_Param6;
        tSharedString                 m_Param7;
        tSharedString                 m_Param8;
        tSharedString                 m_Param9;
        tSharedString                 m_Param10;
        
        //! For Search
        tString                 m_KeyString;

        // Calculate m_Sum
        tDouble                 m_Sum;
        
        // ColorScales calculated values (computed from actual cell values)
        tDouble                 m_CalculatedMinValue;
        tDouble                 m_CalculatedMaxValue;
        tDouble                 m_CalculatedMidValue;
        tBool                   m_HasCalculatedValues;
    public:
        /// @brief      Constructor
        tConditionalFormat();

        /// @brief      Constructor copy 
        tConditionalFormat(const tConditionalFormat&  sConditionalFormat);

        /// @brief      Constructor
        /// @param[in] sConditionalFormatContainer tConditionalFormatContainer*
        /// @param[in] sConditionalFormatType tConditionalFormatType
        /// @param[in] sRef tString
        /// @param[in] sSheet tSheet*
        tConditionalFormat(tConditionalFormatContainer* sConditionalFormatContainer, tConditionalFormatType sConditionalFormatType,tString sRef,tSheet* sSheet);

        /// @brief      Destructor
        ~tConditionalFormat();

        /// @brief      Return conditional format type
        /// @return     tConditionalFormatType
        tConditionalFormatType Type();

        /// @brief      Return icon type
        /// @return     tIconType
        tIconType IconType();

        /// @brief      Set icon type
        /// @param[in]  sIconType tIconType
        void IconType(tIconType sIconType);

        /// @brief      Return  Patam1 ( format true)
        /// @return     tString
        tString Param1();

        /// @brief      Return param 2
        /// @return     tString
        tString Param2();

        /// @brief      Return param 3
        /// @return     tString
        tString Param3();

        /// @brief      Return param 4
        /// @return     tString
        tString Param4();

        /// @brief      Return param 5
        /// @return     tString
        tString Param5();

        /// @brief      Return param 6
        /// @return     tString
        tString Param6();

        /// @brief      Return param 7
        /// @return     tString
        tString Param7();

        /// @brief      Set param 1
        /// @param[in]  sParam1 tString
        void SetParam1(tString sParam1);

        /// @brief      Set param 2
        /// @param[in]  sParam2 tString
        void SetParam2(tString sParam2);

        /// @brief      Set param 3
        /// @param[in]  sParam3 tString
        void SetParam3(tString sParam3);

        /// @brief      Set param 4
        /// @param[in]  sParam4 tString
        void SetParam4(tString sParam4);

        /// @brief      Set param 5
        /// @param[in]  sParam5 tString
        void SetParam5(tString sParam5);

        /// @brief      Set param 6
        /// @param[in]  sParam6 tString
        void SetParam6(tString sParam6);

        /// @brief      Set param 7
        /// @param[in]  sParam7 tString
        void SetParam7(tString sParam7);

        /// @brief      Return param 8
        /// @return     tString
        tString Param8();

        /// @brief      Return param 9
        /// @return     tString
        tString Param9();

        /// @brief      Return param 10
        /// @return     tString
        tString Param10();

        /// @brief      Set param 8
        /// @param[in]  sParam8 tString
        void SetParam8(tString sParam8);

        /// @brief      Set param 9
        /// @param[in]  sParam9 tString
        void SetParam9(tString sParam9);

        /// @brief      Set param 10
        /// @param[in]  sParam10 tString
        void SetParam10(tString sParam10);
        
        /// @brief      Return key
        /// @return     tString
        tString Key();
        
        /// @brief      Return Ref
        /// @return     tString
        tString Ref();
        
        /// @brief      Set key (Retun First Range for unicity)
        /// @param[in]  sKey tString
        /// @param[in]  sSheet tSheet*
        void SetKeyAndEnsureRange(tString m_ref,tSheet* sSheet);

        /// @brief      Return key
        /// @return     tConditionalKey*
        tConditionalRanges* ConditionalRange();

         /// @brief      Clear  Ref and formula
        void ClearVectorRefAndDeleteDependant() override;

        /// @brief      Compil formula
        /// @param[in]  sFormula tString
        /// @param[in]  sFormatTrue tSring
        /// @param[in]  sFormatFalse tString
        /// @return     tBool
        tBool Compil(tString sFormula,tString sFormatTrue,tString sFormatFalse);

        /// @brief      Call back cell
        /// @param[in]  sCell tCell*
        /// @param[in]  sPass tPass
        void CallBackCell(tCell* sCell,tPass sPass);

        /// @brief      Call back range
        /// @param[in]  sRange tRange*
        /// @param[in]  sPass tPass
        void CallBackRange(tRange* sRange,tPass sPass);

        ///@brief Execute
        void Apply(tPass sPass);
        
        // Formula
        /// @brief      Set formula
        /// @param[in]  sFormula tFormula*
        void Formula(const tFormula* sFormula) override;
   
        /// @brief      Return Formula
        tFormula* Formula() override;
        
        /// @brief      Return FormulaStr
        /// @param[in]  sCell tCell*
        tString FormulaStr(tCell *sCell);

        /// @brief      Return sheet
        tSheet* Sheet() override;
        
        /// @brief      Push reference
        /// @param[in]  sItem tItem*
        /// @param[in]  sDependent tBool
        void PushRef(tItem* sItem,tBool sDependent=true) override;
        
             
        /// @brief Return Vector Item
        /// @return tVectorItem*
        tVectorItem* VectorItem() override;

        /// @brief      Add cell conditionnal format
        /// @param[in]  sCell tCell*
        /// @param[in]  sItemConditionnalFormat tItemConditionnalFormat
        void AddCellConditionnalFormat(tCell* sCell, tItemCF sItemConditionnalFormat);

        /// @brief      Return cell conditionnal format
        /// @param[in]  sCell tCell*
        /// @return     tItemConditionnalFormat
        tItemCF CellConditionnalFormat(tCell* sCell);
        
        /// @brief      Return row index (Abstract tInterfaceCompil)
        /// @return     tIndex
        tIndex RowIndex() override;

        /// @brief      Return col index (I Abstract tInterfaceCompil )
        /// @return     tIndex
        tIndex ColIndex() override;

         /// @brief      Intersect rect
        /// @param[in]  sRect tRect*
        /// @return     tBool
        tBool IntersectRect(tRect* sRect);
        
        // Serch
        /// @brief      Set search key string
        /// @param[in]  sKeyString tString
        void KeyString(tString sKeyString);

        /// @brief      Return formula string
        /// @return     tString
        tString FormulaString();

        /// @brief      Calculate ColorScale color for a cell
        /// @param[in]  sCell tCell*
        /// @return     tString (hex color)
        tString CalculateColorScaleColor(tCell* sCell);

        /// @brief      Calculate DataBar percentage for a cell
        /// @param[in]  sCell tCell*
        /// @return     tDouble (percentage 0.0-1.0)
        tDouble CalculateDataBarPercent(tCell* sCell);

        /// @brief      Calculate IconSet CSS for a cell
        /// @param[in]  sCell tCell*
        /// @return     tString (CSS)
        tString CalculateIconSet(tCell* sCell);

        /// @brief      Same as CalculateIconSet, but also returns the 0-based
        ///             position of the selected icon inside the set (0..4) and
        ///             the total number of icons in the set (3 or 5). Used by
        ///             the renderer to pick a tier-specific visual.
        ///             Returns "" / sIndex=-1 / sTotal=0 when the cell value
        ///             is not numeric.
        tString CalculateIconSetWithIndex(tCell* sCell, tInt& sIndex, tInt& sTotal);
        
        /// @brief      Return true if range exist in vector save range
        /// @param[in]  sRange tRange*
        /// @return     tBool
        tBool RangeExist(tRange* sRange);
        
        /// @brief      Remove range
        /// @param[in]  sRange tRange*
        /// @return     tBool
        tBool RemoveRange(tRange* sRange);


        /// @brief      Is Empty
        /// @return     tBool
        tBool IsEmpty();
        
           /// @brief		Writer Json.
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        /// @param[in]	sSheet tSheet*
        void Json(const Value& sValue,tSheet* sSheet);

        /// @brief		Debug.
        /// @return		tString
        tString Debug();
#ifdef checksp
        /// @brief      Check
        void Check();
#endif
    };

    // =========================================================================
    //! Vector of Conditional Format
    typedef vector<tConditionalFormat*> tVectorConditionalFormat;

    // Sort Vector of Conditional Format
    class tComparatorConditionalFormat {
    public:
        /// @brief      Operator() compare with tConditionalFormat < operator.
        /// @param[in]  sE2 tConditionalFormat*
        /// @param[in]  sE1 tConditionalFormat*
        SkInline bool operator()(tConditionalFormat* sE1, tConditionalFormat* sE2) {
            // Debug cout << "CompareConditionalFormat->" <<  sE1->Key() << "<"  << sE2->Key() << endl;
            return(sE1->Key() < sE2->Key());
        }
    };

    class tRangeConditionnalFormat : public tClass {
    private:
        tVectorConditionalFormat m_VectorConditionalFormat;
        // Excel may stack several IconSets on the same range (e.g. 3Signs + 3Symbols2).
        std::vector<tConditionalFormat*> m_MoreIconSets;

        /// @brief      Return index of conditional format
        /// @param[in]  sType tConditionalFormatType
        /// @return     tInt
        tInt Index(tConditionalFormatType sType);
    public:
        /// @brief      Constructor
        tRangeConditionnalFormat();
        /// @brief      Destructor
        ~tRangeConditionnalFormat();
        /// @brief      Return conditional format
        /// @param[in]  sConditionalFormatType tConditionalFormatType
        /// @return     tConditionalFormat*
        tConditionalFormat* ConditionalFormat(tConditionalFormatType sConditionalFormatType);

        /// @brief      Add conditional format
        /// @param[in]  sConditionalFormat tConditionalFormat*
        tBool AddConditionalFormat(tConditionalFormat* sConditionalFormat);

        /// @brief      Delete conditional format
        /// @param[in]  sConditionalFormatType tConditionalFormatType
        tBool DeleteConditionalFormat(tConditionalFormatType sConditionalFormatType);


        /// @brief      Return vector conditional format
        /// @return     tVectorConditionalFormat
        tVectorConditionalFormat& VectorConditionalFormat();

        /// @brief      Is empty
        /// @return     tBool
        tBool IsEmpty();
    };

    // =========================================================================
    //! Cell Conditional Format
    class tCellConditionalFormat : public tClass {
    private:
        tColRowCellRange* m_ColRowCellRange;
        tVectorItemCF m_VectorItemCF;
    public:
        /// @brief      Constructor
        tCellConditionalFormat(tColRowCellRange* sColRowCellRange = nullptr);
        /// @brief      Destructor
        ~tCellConditionalFormat();
        /// @brief      Add cell conditionnal format
        /// @param[in]  sItemCF tAllocatorRef
        void AddItemCF(tAllocatorRef sItemCF);

        /// @brief      Remove item CF
        /// @param[in]  sItemCF tAllocatorRef
        void RemoveItemCF(tAllocatorRef sItemCF);

        /// @brief      Return item CF
        /// @return     tAllocatorRef
        tAllocatorRef ItemCF(tConditionalFormatType sType);
        
        /// @brief      Return vector item CF
        /// @return     tVectorItemCF
        tVectorItemCF& VectorItemCF();

        /// @brief      Is empty
        /// @return     tBool
        tBool IsEmpty();
        
        /// @brief      Serialize to JSON
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);
        
        /// @brief      Deserialize from JSON
        /// @param[in]  sValue Value&
        void Json(const Value& sValue);
        
        /// @brief      Return JSON string
        /// @return     tString
        tString Json();
    };

    // =========================================================================
    //! Conditional Format Container
    class tConditionalFormatContainer : public tClass {
    private:
        // Sheet 
        tSheet*                 m_Sheet;
        
        // Map Conditional Range

        typedef map<tRange*,tRangeConditionnalFormat*> tMapRange;

        // Vector Conditional format
        typedef vector<tConditionalFormat*> tVectorContionalFormat;
        // Vector of Conditional Format
        tVectorContionalFormat      m_VectoConditionalFormat;
        // Intersect Range
        tMapRange                   m_MapRange;
        
        // Vector of condition formal call before jsonView
        tVectorContionalFormat      m_VectorIntersect;

        // Cell container
        typedef map<tCell*, tCellConditionalFormat*> tMapCellConditionnalFormat;
        tMapCellConditionnalFormat  m_MapCell;

        /// JsonView viewport clip — limits per-cell CF Apply/Free to visible band only.
        tRect                       m_JsonViewClipRect;
        tBool                       m_JsonViewClipActive;
    public:
        /// @brief      Constructor
        tConditionalFormatContainer(tSheet* sSheet);

        /// @brief      Destructor
        ~tConditionalFormatContainer();

        /// @brief      Return sheet
        /// @return     tSheet*
        tSheet* Sheet();

        /// @brief      If Conditional Format exist
        /// @param[in]  sRef tString
        /// @return     tBool
        tBool Exist(tString sRef);

        /// @brief      Return Add Rane in Map Range
        /// @param[in]  sConditionalFormatType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @param[in]  sConditionalFormat tConditionalFormat*
        /// @return     tBool
        tBool AddRangeInMapRange(tConditionalFormatType sType, tString sRef,tConditionalFormat* sConditionalFormat);


        /// @brief      Delete Range in Map Range
        /// @param[in]  sRange tRange*
        /// @return     tBool
        tBool DeleteRangeInMapRange(tRange* sRange);

        /// @brief      Add Conditional Format
        /// @param[in]  sConditionalFormatType tConditionalFormatType
        /// @param[in]  sSheet tSheet*
        /// @param[in]  sRef tString
        /// @param[in]  sSheet tSheet*
        /// @return     tConditionalFormat*
        tConditionalFormat* Add(tConditionalFormatType sConditionalFormatType,tString sRef,tSheet* sSheet);

        /// @brief      Empty
        /// @return     tBool
        tBool IsEmpty();

        /// @brief      Remove Conditional Format
        /// @param[in]  sKey tString
        /// @return     tBool///
        tBool RemoveByKey(tString sKey);
        
        /// @brief      Remove Conditional Format
        /// @param[in]  sConditionalFormatType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @param[in]  sAreaDelete tRect*
        /// @return     tBool///
        tBool RemoveByRect(tConditionalFormatType sConditionalFormatType,tString sRef,tRect* sAreaDelete);
        
        // Conditional Format ==================================================
        /// @brief     Add  Conditional Format on Container
        /// @brief     sType tConditionalFormatType
        /// @param[in]  sRef tString
        /// @return     tConditionalFormatContainer*
        tConditionalFormat* ConditionalFormat(tConditionalFormatType sType,tString sRef);
        
        /// @brief Get conditional format by Range
        /// @param[in]  sRange tRange*
        /// @return     tConditionalFormat*
        tRangeConditionnalFormat* ConditionalFormatByRange(tRange* sRange);

        /// @brief      Intersect rect
        /// @param[in]  sRect tRect*
        /// @return     tBool
        tBool MakeIntersectRectList(tRect* sRect);
        
        
        /// @brief      Apply before  JsonView (make celle
        void ApplyJsonView();

        /// @brief      True while JsonView CF apply should clip to m_JsonViewClipRect.
        tBool JsonViewClipActive() const;

        /// @brief      Viewport sheet rect set by MakeIntersectRectList for JsonView CF.
        const tRect& JsonViewClipRect() const;

        /// @brief      Apply free
        void ApplyFree();

        /// @brief      Release applied ItemCF CSS refs (sheet/workbook teardown).
        void ReleaseAppliedFormats();

        /// @brief      Return number of Conditional Format
        /// @return     tSize
        tSize Size();

        /// @brief      Indexed access for export / tooling (0 .. Size()-1).
        tConditionalFormat* ConditionalFormatAt(tSize sIndex);

        // Cell container =====================================================
        /// @brief      Add item CF
        /// @param[in]  sCell tCell*
        /// @param[in]  sItemCF tAllocatorRef
        void AddItemCF(tCell* sCell, tAllocatorRef sItemCF);
        
        /// @brief      Return item CF
        /// @param[in]  sCell tCell*
        /// @return     tCellConditionalFormat
        tCellConditionalFormat* CellIConditionalFormat(tCell* sCell);

        /// @brief True when JsonView already has an IconSets ItemCF on this cell.
        tBool CellHasIconSetItemCF(tCell* sCell);

        /// @brief Remove IconSets ItemCF on a cell so a higher-priority rule can replace it.
        void RemoveIconSetItemCF(tCell* sCell);


        //Json ================================================================
           /// @brief		Writer Json.
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        /// @param[in]	sSheet tSheet*
        void Json(const Value& sValue,tSheet* sSheet);

        /// @brief      Return JSON conditional format.
        /// @return     tString
        tString Json();

        /// @brief      Return JSON conditional format by intersect rect.
        /// @param[in]  sRect tRect*
        /// @return     tString
        tString JsonByRect(tRect* sRect);

        /// @brief		Debug.
            /// @return		tString
        tString Debug();
#ifdef checksp
        // @brief        CheckRange
        void CheckRange(tRange* sRange);
        /// @brief      Check
        void Check();
#endif
    };

    //=========================================================================
    //! Utility function to convert ConditionalFormatType to string
    //=========================================================================
    const char* ConditionalFormatTypeToString(tConditionalFormatType type);

    //=========================================================================
    //! Utility function to convert string to ConditionalFormatType
    //=========================================================================
    tConditionalFormatType StringToConditionalFormatType(const char* typeString);


    //=========================================================================
    //! Utility function to convert string to get Key Conditional format
    //=========================================================================
    tString  KeyStyleRef(tConditionalFormatType sType,tString sRef);

    //=========================================================================
    //! Utility function to convert IconType to string
    //=========================================================================
    const char* IconTypeToString(tIconType type);

    //=========================================================================
    //! Utility function to convert string to IconType
    //=========================================================================
    tIconType StringToIconType(const char* typeString);
}


#endif
