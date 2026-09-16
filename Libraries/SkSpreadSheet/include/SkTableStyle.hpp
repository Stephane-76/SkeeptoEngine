//=============================================================================
// SkSpreadSheet — Excel ListObject table styles (tFormatRef overlays)
//=============================================================================
#ifndef SkTableStyle_hpp
#define SkTableStyle_hpp

#include "SkTools.hpp"
#include <map>
#include <memory>
#include <vector>

namespace SkRoot {
    class tFormatApi;
}

namespace SkSpreadSheet {

    using SkRoot::tFormatApi;

    class tRange;
    class tRangeData;
    class tSheet;
    class tWorkBook;

    //! OOXML tableStyleElement type names (subset used by sker).
    namespace TableStyleElement {
        inline constexpr const char* kWholeTable = "wholeTable";
        inline constexpr const char* kHeaderRow = "headerRow";
        //! OOXML ST_TableStyleType spelling (not totalsRow).
        inline constexpr const char* kTotalRow = "totalRow";
        inline constexpr const char* kFirstRowStripe = "firstRowStripe";
        inline constexpr const char* kSecondRowStripe = "secondRowStripe";
        inline constexpr const char* kFirstColumnStripe = "firstColumnStripe";
        inline constexpr const char* kSecondColumnStripe = "secondColumnStripe";
        inline constexpr const char* kFirstColumn = "firstColumn";
        inline constexpr const char* kLastColumn = "lastColumn";
    }

    //=========================================================================
    //! One named Excel table style: element fragments compiled to tFormatRef.
    class tTableStyle : public tClass {
    private:
        tString m_Name;
        tBool m_ShowRowStripes = true;
        tBool m_ShowColumnStripes = false;
        tBool m_ShowFirstColumn = false;
        tBool m_ShowLastColumn = false;

        //! FormatApi used to IncCell / DeleteCellFormat for owned refs (ReleaseFormats / destructor).
        tFormatApi* m_FormatApi = nullptr;

        std::map<tString, tFormatRef> m_ElementFormat;
        tFormatRef m_HeaderOverlay = 0;
        tFormatRef m_TotalsOverlay = 0;
        tFormatRef m_DataStripeEvenOverlay = 0;
        tFormatRef m_DataStripeOddOverlay = 0;
        tFormatRef m_WholeTableOverlay = 0;

        void PrecomputeRoleOverlays(tFormatApi* sFormatApi);

    public:
        explicit tTableStyle(tString sName);
        ~tTableStyle();

        tString Name() const { return m_Name; }

        void SetShowRowStripes(tBool sValue) { m_ShowRowStripes = sValue; }
        void SetShowColumnStripes(tBool sValue) { m_ShowColumnStripes = sValue; }
        void SetShowFirstColumn(tBool sValue) { m_ShowFirstColumn = sValue; }
        void SetShowLastColumn(tBool sValue) { m_ShowLastColumn = sValue; }

        tBool ShowRowStripes() const { return m_ShowRowStripes; }
        tBool ShowColumnStripes() const { return m_ShowColumnStripes; }
        tBool ShowFirstColumn() const { return m_ShowFirstColumn; }
        tBool ShowLastColumn() const { return m_ShowLastColumn; }

        void SetElementFormat(const tString& sType, tFormatRef sFormatRef);
        tFormatRef ElementFormat(const tString& sType) const;

		//! Release every owned ApplyCellFormat / ApplyMerge / OwnOverlayRef slot.
        void ReleaseFormats(tFormatApi* sFormatApi);

#ifdef checkfo
        //! Mirror CollectTableStyleFormatOwners counts for CheckFormat.
        void IncCheckFormats(tFormatApi* sFormatApi) const;
#endif

        //! Compile element CSS strings then build merged role overlays.
        void BuildFromElementCss(tFormatApi* sFormatApi,
                                 const std::map<tString, tString>& sElementCss);

        //! Merged overlay for one cell inside a structured table.
        tFormatRef OverlayFormat(tFormatApi* sFormatApi,
                                 tIndex sRow,
                                 tIndex sCol,
                                 tRange* sRange,
                                 tRangeData* sRangeData) const;

        //! Role overlay + optional column/first/last element refs (no ApplyMerge).
        void AppendOverlayFormats(tFormatApi* sFormatApi,
                                  tIndex sRow,
                                  tIndex sCol,
                                  tRange* sRange,
                                  tRangeData* sRangeData,
                                  std::vector<tFormatRef>& sOut) const;
    };

    //=========================================================================
    //! Workbook-owned registry of built-in and custom table styles.
    class tTableStyleContainer : public tClass {
    private:
        tWorkBook* m_WorkBook = nullptr;
        tBool m_BuiltinsInitialized = false;
        std::map<tString, std::unique_ptr<tTableStyle>> m_Styles;

        tFormatRef CompileElementCss(tFormatApi* sFormatApi, const tString& sCss);
        void InitBuiltins();
        void RegisterBuiltinMedium(tInt sStyleNumber);
        void RegisterBuiltinLight(tInt sStyleNumber);
        void RegisterBuiltinDark(tInt sStyleNumber);
        //! Compile one built-in on demand (avoids IncCell on all 60 styles at once).
        tTableStyle* EnsureBuiltinStyle(const tString& sName);

    public:
        explicit tTableStyleContainer(tWorkBook* sWorkBook = nullptr);

        void SetWorkBook(tWorkBook* sWorkBook) { m_WorkBook = sWorkBook; }
        tWorkBook* WorkBook() const { return m_WorkBook; }

        void EnsureBuiltins();

        tTableStyle* Find(const tString& sName);
        const tTableStyle* Find(const tString& sName) const;

        tTableStyle* RegisterFromElementCss(const tString& sName,
                                            const std::map<tString, tString>& sElementCss);

        tTableStyle* RegisterFromRangeData(const tString& sFallbackName,
                                           const tRangeData* sRangeData);

        //! Resolve ListObject overlay for JsonView / paint (0 when not in a table).
        tFormatRef OverlayForCell(tSheet* sSheet, tIndex sRow, tIndex sCol);

        //! Role + column/first/last overlays as separate FormatRefs (no ApplyMerge).
        void AppendOverlayFormats(tSheet* sSheet,
                                  tIndex sRow,
                                  tIndex sCol,
                                  std::vector<tFormatRef>& sOut);

        //! CSS for one OOXML tableStyleElement on a registered style ("" if unknown).
        tString ElementCssString(const tString& sName, const tString& sType) const;

        //! Merged table overlay as CSS (export / legacy string paths).
        tString OverlayCssString(tSheet* sSheet, tIndex sRow, tIndex sCol);

        void Clear();

#ifdef checkfo
        void CheckFormat();
#endif
    };

} // namespace SkSpreadSheet

#endif /* SkTableStyle_hpp */
