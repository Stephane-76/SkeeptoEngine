//=============================================================================
// SkSpreadSheet — Excel ListObject table styles (tFormatRef overlays)
//=============================================================================
#include "../include/SkTableStyle.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkRange.hpp"
#include "../include/SkRangeData.hpp"
#include <SkFormatApi.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace SkSpreadSheet {

    namespace {

        static const char* const kCoreElementTypes[] = {
            TableStyleElement::kWholeTable,
            TableStyleElement::kHeaderRow,
            TableStyleElement::kTotalRow,
            TableStyleElement::kFirstRowStripe,
            TableStyleElement::kSecondRowStripe,
            TableStyleElement::kFirstColumnStripe,
            TableStyleElement::kSecondColumnStripe,
            TableStyleElement::kFirstColumn,
            TableStyleElement::kLastColumn,
        };

        // Each map entry and each overlay slot owns one IncCell (ReleaseFormats / checkfo).
        // Overlay slots always IncCellFormat — even when they alias an element ref — so
        // ApplyMerge pool-hits and wholeTable aliases stay balanced with Collect counts.
        static void CollectTableStyleFormatOwners(
            const std::map<tString, tFormatRef>& sElementFormat,
            tFormatRef sWholeTableOverlay,
            tFormatRef sHeaderOverlay,
            tFormatRef sTotalsOverlay,
            tFormatRef sDataStripeEvenOverlay,
            tFormatRef sDataStripeOddOverlay,
            std::map<tFormatRef, tInt>& sOutCounts) {
            sOutCounts.clear();
            for (const auto& wEntry : sElementFormat) {
                if (wEntry.second != 0) {
                    sOutCounts[wEntry.second]++;
                }
            }
            auto wAddOverlay = [&](tFormatRef sRef) {
                if (sRef == 0) {
                    return;
                }
                sOutCounts[sRef]++;
            };
            wAddOverlay(sWholeTableOverlay);
            wAddOverlay(sHeaderOverlay);
            wAddOverlay(sTotalsOverlay);
            wAddOverlay(sDataStripeEvenOverlay);
            wAddOverlay(sDataStripeOddOverlay);
        }

        //! Own one overlay slot (IncCell) so ReleaseFormats / checkfo match.
        static tFormatRef OwnOverlayRef(tFormatApi* sFormatApi, tFormatRef sRef) {
            if (sFormatApi == nullptr || sRef == 0) {
                return 0;
            }
            sFormatApi->IncCellFormat(sRef);
            return sRef;
        }

        static tFormatRef MergeFormatLayers(tFormatApi* sFormatApi,
                                            const std::vector<tFormatRef>& sLayers) {
            if (sFormatApi == nullptr) {
                return 0;
            }
            tVectorFormatRef wVector;
            for (tFormatRef wRef : sLayers) {
                if (wRef != 0) {
                    wVector.push_back(wRef);
                }
            }
            if (wVector.empty()) {
                return 0;
            }
            if (wVector.size() == 1) {
                // Alias of a single element — still take an overlay ownership slot.
                return OwnOverlayRef(sFormatApi, wVector.front());
            }
            sFormatApi->BeginMerge();
            for (tFormatRef wRef : wVector) {
                sFormatApi->Merge(wRef);
            }
            // ApplyMerge → ApplyCell already IncCell once for the merged (or pool-hit) ref.
            return sFormatApi->ApplyMerge();
        }

        //! Office default theme (clrScheme order) — matches a fresh Excel workbook.
        static tString OfficeThemeColor(tInt sThemeIndex) {
            static const char* const kOfficeTheme[] = {
                "FFFFFF", // lt1
                "000000", // dk1
                "E7E6E6", // lt2
                "44546A", // dk2
                "4472C4", // accent1
                "ED7D31", // accent2
                "A5A5A5", // accent3
                "FFC000", // accent4
                "5B9BD5", // accent5
                "70AD47", // accent6
            };
            if (sThemeIndex < 0
                || sThemeIndex >= static_cast<tInt>(sizeof(kOfficeTheme) / sizeof(kOfficeTheme[0]))) {
                return "";
            }
            return kOfficeTheme[sThemeIndex];
        }

        static tString ApplyTintToSrgb(const tString& sRgb, tDouble sTint) {
            if (sRgb.size() != 6) {
                return sRgb;
            }
            auto wHex = [](tChar sChar) -> tInt {
                if ('0' <= sChar && sChar <= '9') {
                    return sChar - '0';
                }
                if ('A' <= sChar && sChar <= 'F') {
                    return sChar - 'A' + 10;
                }
                if ('a' <= sChar && sChar <= 'f') {
                    return sChar - 'a' + 10;
                }
                return 0;
            };
            tInt wR = (wHex(sRgb[0]) << 4) + wHex(sRgb[1]);
            tInt wG = (wHex(sRgb[2]) << 4) + wHex(sRgb[3]);
            tInt wB = (wHex(sRgb[4]) << 4) + wHex(sRgb[5]);
            auto wApply = [&](tInt sChannel) -> tInt {
                tDouble wValue = sChannel / 255.0;
                if (sTint > 0.0) {
                    wValue = wValue * (1.0 - sTint) + 1.0 * sTint;
                } else {
                    wValue = wValue * (1.0 + sTint);
                }
                const tInt wOut = static_cast<tInt>(std::round(std::clamp(wValue, 0.0, 1.0) * 255.0));
                return std::clamp(wOut, 0, 255);
            };
            wR = wApply(wR);
            wG = wApply(wG);
            wB = wApply(wB);
            char wBuf[7];
            std::snprintf(wBuf, sizeof(wBuf), "%02X%02X%02X", wR, wG, wB);
            return tString(wBuf);
        }

        //! Accent index: 0 = neutral (dk1), 1..6 = accent1..accent6.
        static tString ThemeAccentRgb(tInt sAccentIndex, tDouble sTint) {
            tString wRgb;
            if (sAccentIndex <= 0) {
                wRgb = OfficeThemeColor(1);
            } else {
                wRgb = OfficeThemeColor(3 + sAccentIndex);
            }
            if (wRgb.empty()) {
                return "";
            }
            if (sTint != 0.0) {
                wRgb = ApplyTintToSrgb(wRgb, sTint);
            }
            return wRgb;
        }

        enum class BuiltinStyleFamily { None, Light, Medium, Dark };

        static tBool ParseBuiltinStyleName(const tString& sName,
                                           BuiltinStyleFamily& sFamily,
                                           tInt& sNumber) {
            sFamily = BuiltinStyleFamily::None;
            sNumber = 0;
            const char* wPrefix = "TableStyle";
            const tSize wPrefixLen = std::strlen(wPrefix);
            if (sName.size() <= wPrefixLen || sName.compare(0, wPrefixLen, wPrefix) != 0) {
                return false;
            }
            tString wRest = sName.substr(wPrefixLen);
            if (wRest.compare(0, 5, "Light") == 0) {
                sFamily = BuiltinStyleFamily::Light;
                wRest = wRest.substr(5);
            } else if (wRest.compare(0, 6, "Medium") == 0) {
                sFamily = BuiltinStyleFamily::Medium;
                wRest = wRest.substr(6);
            } else if (wRest.compare(0, 4, "Dark") == 0) {
                sFamily = BuiltinStyleFamily::Dark;
                wRest = wRest.substr(4);
            } else {
                return false;
            }
            if (wRest.empty()) {
                return false;
            }
            for (tChar wChar : wRest) {
                if (wChar < '0' || wChar > '9') {
                    return false;
                }
            }
            sNumber = std::atoi(wRest.c_str());
            return sNumber > 0;
        }

        //! Mirrors SkExcelPugiXMLReader::BuildBuiltinTableStyleElementCss (theme + tint).
        static tString BuildBuiltinTableStyleElementCss(const tString& sStyleName,
                                                        const tString& sElementType) {
            BuiltinStyleFamily wFamily = BuiltinStyleFamily::None;
            tInt wNumber = 0;
            if (!ParseBuiltinStyleName(sStyleName, wFamily, wNumber)) {
                return "";
            }
            const tInt wOffset = ((wNumber - 1) % 7) + 1;
            const tInt wAccentIdx = wOffset - 1;

            if (wFamily == BuiltinStyleFamily::Light) {
                if (sElementType == TableStyleElement::kHeaderRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-bottom:solid 2px #" << wRgb << ";";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kTotalRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-top:solid 2px #" << wRgb << ";";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kFirstRowStripe && wNumber >= 8) {
                    // Explicit white so baked cell fills from import do not show through.
                    return "background-color:#FFFFFF;";
                }
                if (sElementType == TableStyleElement::kSecondRowStripe && wNumber >= 8) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.8);
                    if (wRgb.empty()) {
                        return "";
                    }
                    return tString("background-color:#") + wRgb + ";";
                }
                if (sElementType == TableStyleElement::kFirstColumn
                    || sElementType == TableStyleElement::kLastColumn) {
                    return "font-weight:bold;";
                }
                if (sElementType == TableStyleElement::kFirstColumnStripe) {
                    return "background-color:#FFFFFF;";
                }
                if (sElementType == TableStyleElement::kSecondColumnStripe) {
                    return "background-color:#F2F2F2;";
                }
                return "";
            }

            if (wFamily == BuiltinStyleFamily::Medium) {
                if (sElementType == TableStyleElement::kHeaderRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "background-color:#" << wRgb << ";";
                    wCss << "color:#FFFFFF;";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kTotalRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-top:solid 2px #" << wRgb << ";";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kFirstRowStripe) {
                    // Explicit white clears cell fills baked by ApplyTableStyleOverlays on import.
                    return "background-color:#FFFFFF;";
                }
                if (sElementType == TableStyleElement::kSecondRowStripe) {
                    const tString wRgb = wAccentIdx <= 0
                        ? tString("F2F2F2")
                        : ThemeAccentRgb(wAccentIdx, 0.6);
                    if (wRgb.empty()) {
                        return "";
                    }
                    return tString("background-color:#") + wRgb + ";";
                }
                if (sElementType == TableStyleElement::kFirstColumn
                    || sElementType == TableStyleElement::kLastColumn) {
                    return "font-weight:bold;";
                }
                if (sElementType == TableStyleElement::kFirstColumnStripe) {
                    return "background-color:#FFFFFF;";
                }
                if (sElementType == TableStyleElement::kSecondColumnStripe) {
                    const tString wRgb = wAccentIdx <= 0
                        ? tString("F2F2F2")
                        : ThemeAccentRgb(wAccentIdx, 0.8);
                    if (wRgb.empty()) {
                        return "";
                    }
                    return tString("background-color:#") + wRgb + ";";
                }
                if (sElementType == TableStyleElement::kWholeTable) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-left:solid 1px #" << wRgb << ";";
                    wCss << "border-right:solid 1px #" << wRgb << ";";
                    wCss << "border-top:solid 1px #" << wRgb << ";";
                    wCss << "border-bottom:solid 1px #" << wRgb << ";";
                    return wCss.str();
                }
                return "";
            }

            if (wFamily == BuiltinStyleFamily::Dark) {
                if (sElementType == TableStyleElement::kHeaderRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, -0.5);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "background-color:#" << wRgb << ";";
                    wCss << "color:#FFFFFF;";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kTotalRow) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.0);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-top:solid 2px #" << wRgb << ";";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kSecondRowStripe) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.4);
                    if (wRgb.empty()) {
                        return "";
                    }
                    return tString("background-color:#") + wRgb + ";";
                }
                if (sElementType == TableStyleElement::kFirstColumn
                    || sElementType == TableStyleElement::kLastColumn) {
                    // Dark fill required: white text alone is invisible on white/light stripes.
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, -0.5);
                    if (wRgb.empty()) {
                        return "font-weight:bold;";
                    }
                    std::ostringstream wCss;
                    wCss << "background-color:#" << wRgb << ";";
                    wCss << "color:#FFFFFF;";
                    wCss << "font-weight:bold;";
                    return wCss.str();
                }
                if (sElementType == TableStyleElement::kFirstColumnStripe) {
                    return "background-color:#FFFFFF;";
                }
                if (sElementType == TableStyleElement::kSecondColumnStripe) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, 0.6);
                    if (wRgb.empty()) {
                        return "";
                    }
                    return tString("background-color:#") + wRgb + ";";
                }
                if (sElementType == TableStyleElement::kWholeTable) {
                    const tString wRgb = ThemeAccentRgb(wAccentIdx, -0.5);
                    if (wRgb.empty()) {
                        return "";
                    }
                    std::ostringstream wCss;
                    wCss << "border-left:solid 1px #" << wRgb << ";";
                    wCss << "border-right:solid 1px #" << wRgb << ";";
                    wCss << "border-top:solid 1px #" << wRgb << ";";
                    wCss << "border-bottom:solid 1px #" << wRgb << ";";
                    return wCss.str();
                }
                return "";
            }

            return "";
        }

        static tString BuildBuiltinMediumElementCss(tInt sStyleNumber,
                                                    const tString& sElementType) {
            tStringStream wName;
            wName << "TableStyleMedium" << sStyleNumber;
            return BuildBuiltinTableStyleElementCss(wName.str(), sElementType);
        }

        static tString BuildBuiltinLightElementCss(tInt sStyleNumber,
                                                   const tString& sElementType) {
            tStringStream wName;
            wName << "TableStyleLight" << sStyleNumber;
            return BuildBuiltinTableStyleElementCss(wName.str(), sElementType);
        }

        static tString BuildBuiltinDarkElementCss(tInt sStyleNumber,
                                                  const tString& sElementType) {
            tStringStream wName;
            wName << "TableStyleDark" << sStyleNumber;
            return BuildBuiltinTableStyleElementCss(wName.str(), sElementType);
        }

        static tBool IsBuiltinExcelTableStyleName(const tString& sName) {
            BuiltinStyleFamily wFamily = BuiltinStyleFamily::None;
            tInt wNumber = 0;
            return ParseBuiltinStyleName(sName, wFamily, wNumber);
        }

    } // namespace

    // tTableStyle =============================================================

    tTableStyle::tTableStyle(tString sName) : tClass(), m_Name(std::move(sName)) {}

    tTableStyle::~tTableStyle() {
        // Guard against map erase / unique_ptr teardown without an explicit Clear().
        ReleaseFormats(m_FormatApi);
    }

    void tTableStyle::SetElementFormat(const tString& sType, tFormatRef sFormatRef) {
        if (sFormatRef == 0) {
            return;
        }
        m_ElementFormat[sType] = sFormatRef;
    }

    tFormatRef tTableStyle::ElementFormat(const tString& sType) const {
        auto wIt = m_ElementFormat.find(sType);
        if (wIt == m_ElementFormat.end()) {
            return 0;
        }
        return wIt->second;
    }

    void tTableStyle::ReleaseFormats(tFormatApi* sFormatApi) {
        tFormatApi* wApi = sFormatApi != nullptr ? sFormatApi : m_FormatApi;
        if (wApi == nullptr) {
            m_ElementFormat.clear();
            m_WholeTableOverlay = 0;
            m_HeaderOverlay = 0;
            m_TotalsOverlay = 0;
            m_DataStripeEvenOverlay = 0;
            m_DataStripeOddOverlay = 0;
            m_FormatApi = nullptr;
            return;
        }
        std::map<tFormatRef, tInt> wRefCounts;
        CollectTableStyleFormatOwners(
            m_ElementFormat,
            m_WholeTableOverlay,
            m_HeaderOverlay,
            m_TotalsOverlay,
            m_DataStripeEvenOverlay,
            m_DataStripeOddOverlay,
            wRefCounts);
        for (const auto& wEntry : wRefCounts) {
            for (tInt wI = 0; wI < wEntry.second; ++wI) {
                wApi->DeleteCellFormat(wEntry.first);
            }
        }
        m_ElementFormat.clear();
        m_WholeTableOverlay = 0;
        m_HeaderOverlay = 0;
        m_TotalsOverlay = 0;
        m_DataStripeEvenOverlay = 0;
        m_DataStripeOddOverlay = 0;
        m_FormatApi = nullptr;
    }

#ifdef checkfo
    void tTableStyle::IncCheckFormats(tFormatApi* sFormatApi) const {
        if (sFormatApi == nullptr) {
            return;
        }
        std::map<tFormatRef, tInt> wRefCounts;
        CollectTableStyleFormatOwners(
            m_ElementFormat,
            m_WholeTableOverlay,
            m_HeaderOverlay,
            m_TotalsOverlay,
            m_DataStripeEvenOverlay,
            m_DataStripeOddOverlay,
            wRefCounts);
        for (const auto& wEntry : wRefCounts) {
            for (tInt wI = 0; wI < wEntry.second; ++wI) {
                sFormatApi->IncCheck(wEntry.first);
            }
        }
    }
#endif

    void tTableStyle::BuildFromElementCss(tFormatApi* sFormatApi,
                                          const std::map<tString, tString>& sElementCss) {
        ReleaseFormats(sFormatApi);
        m_FormatApi = sFormatApi;
        for (const auto& wEntry : sElementCss) {
            if (sFormatApi == nullptr || wEntry.second.empty()) {
                continue;
            }
            tFormatRef wRef = sFormatApi->ApplyCellFormat(wEntry.second);
            if (wRef != 0) {
                m_ElementFormat[wEntry.first] = wRef;
            }
        }
        PrecomputeRoleOverlays(sFormatApi);
    }

    void tTableStyle::PrecomputeRoleOverlays(tFormatApi* sFormatApi) {
        const tFormatRef wWhole = ElementFormat(TableStyleElement::kWholeTable);
        const tFormatRef wHeader = ElementFormat(TableStyleElement::kHeaderRow);
        const tFormatRef wTotals = ElementFormat(TableStyleElement::kTotalRow);
        const tFormatRef wStripeFirst = ElementFormat(TableStyleElement::kFirstRowStripe);
        const tFormatRef wStripeSecond = ElementFormat(TableStyleElement::kSecondRowStripe);

        // Every overlay slot owns one IncCell (see OwnOverlayRef / MergeFormatLayers).
        m_WholeTableOverlay = OwnOverlayRef(sFormatApi, wWhole);
        m_HeaderOverlay = MergeFormatLayers(sFormatApi, { wWhole, wHeader });
        m_TotalsOverlay = MergeFormatLayers(sFormatApi, { wWhole, wTotals });
        m_DataStripeEvenOverlay = MergeFormatLayers(sFormatApi, { wWhole, wStripeFirst });
        m_DataStripeOddOverlay = MergeFormatLayers(sFormatApi, { wWhole, wStripeSecond });
        if (m_DataStripeEvenOverlay == 0) {
            m_DataStripeEvenOverlay = OwnOverlayRef(sFormatApi, wWhole);
        }
        if (m_DataStripeOddOverlay == 0) {
            m_DataStripeOddOverlay = OwnOverlayRef(sFormatApi, wWhole);
        }
    }

    tFormatRef tTableStyle::OverlayFormat(tFormatApi* sFormatApi,
                                          tIndex sRow,
                                          tIndex sCol,
                                          tRange* sRange,
                                          tRangeData* sRangeData) const {
        if (sFormatApi == nullptr || sRange == nullptr || sRangeData == nullptr) {
            return 0;
        }

        const tBool wShowRowStripes = sRangeData->TableShowRowStripes();
        const tBool wShowColumnStripes = sRangeData->TableShowColumnStripes();
        const tBool wShowFirstColumn = sRangeData->TableShowFirstColumn();
        const tBool wShowLastColumn = sRangeData->TableShowLastColumn();

        const tIndex wTop = sRange->TopIndex();
        const tIndex wBottom = sRange->BottomIndex();
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wRight = sRange->RightIndex();
        const tBool wHasHeader = sRangeData->HasHeaders();
        const tInt wHeaderRow = wHasHeader ? static_cast<tInt>(wTop) : -1;
        const tInt wDataTop = wHasHeader ? static_cast<tInt>(wTop) + 1 : static_cast<tInt>(wTop);
        const tInt wTotalsRow = sRangeData->HasTotals() ? static_cast<tInt>(wBottom) + 1 : -1;

        tFormatRef wBase = 0;
        if (wHeaderRow >= 0 && static_cast<tInt>(sRow) == wHeaderRow) {
            wBase = m_HeaderOverlay != 0 ? m_HeaderOverlay : m_WholeTableOverlay;
        } else if (wTotalsRow >= 0 && static_cast<tInt>(sRow) == wTotalsRow) {
            wBase = m_TotalsOverlay != 0 ? m_TotalsOverlay : m_WholeTableOverlay;
        } else if (sRow >= static_cast<tIndex>(wDataTop) && sRow <= wBottom) {
            if (wShowRowStripes) {
                const tInt wStripeIdx = static_cast<tInt>(sRow) - wDataTop;
                wBase = (wStripeIdx % 2 == 0) ? m_DataStripeEvenOverlay : m_DataStripeOddOverlay;
            } else {
                wBase = m_WholeTableOverlay;
            }
        } else {
            return 0;
        }

        // Do not ApplyMerge at paint time: each call IncCell without an owner.
        // Callers that need column / first / last extras should AppendOverlayFormats.
        (void)sFormatApi;
        (void)wShowColumnStripes;
        (void)wShowFirstColumn;
        (void)wShowLastColumn;
        (void)wLeft;
        (void)wRight;
        (void)sCol;
        return wBase;
    }

    void tTableStyle::AppendOverlayFormats(tFormatApi* sFormatApi,
                                           tIndex sRow,
                                           tIndex sCol,
                                           tRange* sRange,
                                           tRangeData* sRangeData,
                                           std::vector<tFormatRef>& sOut) const {
        if (sRange == nullptr || sRangeData == nullptr) {
            return;
        }
        const tIndex wTop = sRange->TopIndex();
        const tIndex wBottom = sRange->BottomIndex();
        const tIndex wLeft = sRange->LeftIndex();
        const tIndex wRight = sRange->RightIndex();
        const tBool wHasHeader = sRangeData->HasHeaders();
        const tInt wHeaderRow = wHasHeader ? static_cast<tInt>(wTop) : -1;
        const tInt wDataTop = wHasHeader ? static_cast<tInt>(wTop) + 1 : static_cast<tInt>(wTop);
        const tInt wTotalsRow = sRangeData->HasTotals() ? static_cast<tInt>(wBottom) + 1 : -1;

        // Header / totals: single role overlay (no banded stripes).
        if (wHeaderRow >= 0 && static_cast<tInt>(sRow) == wHeaderRow) {
            const tFormatRef wHeader = OverlayFormat(sFormatApi, sRow, sCol, sRange, sRangeData);
            if (wHeader != 0) {
                sOut.push_back(wHeader);
            }
            return;
        }
        if (wTotalsRow >= 0 && static_cast<tInt>(sRow) == wTotalsRow) {
            const tFormatRef wTotals = OverlayFormat(sFormatApi, sRow, sCol, sRange, sRangeData);
            if (wTotals != 0) {
                sOut.push_back(wTotals);
            }
            return;
        }
        if (sRow < static_cast<tIndex>(wDataTop) || sRow > wBottom) {
            return;
        }

        const tBool wShowRowStripes = sRangeData->TableShowRowStripes();
        const tBool wShowColumnStripes = sRangeData->TableShowColumnStripes();
        const tInt wRowParity = static_cast<tInt>(sRow) - wDataTop;
        const tInt wColParity = static_cast<tInt>(sCol) - static_cast<tInt>(wLeft);

        tFormatRef wBase = 0;
        if (wShowRowStripes && wShowColumnStripes) {
            // Checkerboard so both options stay visible (column stripes must not
            // paint an opaque fill over every row-stripe cell).
            const tBool wShade = ((wRowParity + wColParity) % 2) != 0;
            wBase = wShade ? m_DataStripeOddOverlay : m_DataStripeEvenOverlay;
            if (wBase == 0) {
                wBase = m_WholeTableOverlay;
            }
        } else if (wShowRowStripes) {
            wBase = OverlayFormat(sFormatApi, sRow, sCol, sRange, sRangeData);
        } else {
            wBase = m_WholeTableOverlay != 0
                ? m_WholeTableOverlay
                : OverlayFormat(sFormatApi, sRow, sCol, sRange, sRangeData);
        }
        if (wBase != 0) {
            sOut.push_back(wBase);
        }

        // Column stripe layers only when row stripes are off — otherwise the
        // checkerboard above already encodes both axes.
        if (wShowColumnStripes && !wShowRowStripes) {
            const tFormatRef wColStripe = ((wColParity % 2) == 0)
                ? ElementFormat(TableStyleElement::kFirstColumnStripe)
                : ElementFormat(TableStyleElement::kSecondColumnStripe);
            if (wColStripe != 0) {
                sOut.push_back(wColStripe);
            }
        }

        if (sRangeData->TableShowFirstColumn() && sCol == wLeft) {
            const tFormatRef wFirstCol = ElementFormat(TableStyleElement::kFirstColumn);
            if (wFirstCol != 0) {
                sOut.push_back(wFirstCol);
            }
        }
        if (sRangeData->TableShowLastColumn() && sCol == wRight) {
            const tFormatRef wLastCol = ElementFormat(TableStyleElement::kLastColumn);
            if (wLastCol != 0) {
                sOut.push_back(wLastCol);
            }
        }
    }

    // tTableStyleContainer ====================================================

    tTableStyleContainer::tTableStyleContainer(tWorkBook* sWorkBook)
        : tClass(), m_WorkBook(sWorkBook) {}

    tFormatRef tTableStyleContainer::CompileElementCss(tFormatApi* sFormatApi,
                                                      const tString& sCss) {
        if (sFormatApi == nullptr || sCss.empty()) {
            return 0;
        }
        return sFormatApi->ApplyCellFormat(sCss);
    }

    void tTableStyleContainer::RegisterBuiltinMedium(tInt sStyleNumber) {
        if (m_WorkBook == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = m_WorkBook->FormatApi();
        if (wFormatApi == nullptr) {
            return;
        }
        tStringStream wName;
        wName << "TableStyleMedium" << sStyleNumber;
        std::map<tString, tString> wElements;
        for (const char* wType : kCoreElementTypes) {
            tString wCss = BuildBuiltinMediumElementCss(sStyleNumber, wType);
            if (!wCss.empty()) {
                wElements[wType] = wCss;
            }
        }
        auto wStyle = std::make_unique<tTableStyle>(wName.str());
        wStyle->SetShowRowStripes(true);
        wStyle->BuildFromElementCss(wFormatApi, wElements);
        m_Styles[wName.str()] = std::move(wStyle);
    }

    void tTableStyleContainer::RegisterBuiltinLight(tInt sStyleNumber) {
        if (m_WorkBook == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = m_WorkBook->FormatApi();
        if (wFormatApi == nullptr) {
            return;
        }
        tStringStream wName;
        wName << "TableStyleLight" << sStyleNumber;
        std::map<tString, tString> wElements;
        for (const char* wType : kCoreElementTypes) {
            tString wCss = BuildBuiltinLightElementCss(sStyleNumber, wType);
            if (!wCss.empty()) {
                wElements[wType] = wCss;
            }
        }
        auto wStyle = std::make_unique<tTableStyle>(wName.str());
        wStyle->SetShowRowStripes(sStyleNumber >= 8);
        wStyle->BuildFromElementCss(wFormatApi, wElements);
        m_Styles[wName.str()] = std::move(wStyle);
    }

    void tTableStyleContainer::RegisterBuiltinDark(tInt sStyleNumber) {
        if (m_WorkBook == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = m_WorkBook->FormatApi();
        if (wFormatApi == nullptr) {
            return;
        }
        tStringStream wName;
        wName << "TableStyleDark" << sStyleNumber;
        std::map<tString, tString> wElements;
        for (const char* wType : kCoreElementTypes) {
            tString wCss = BuildBuiltinDarkElementCss(sStyleNumber, wType);
            if (!wCss.empty()) {
                wElements[wType] = wCss;
            }
        }
        auto wStyle = std::make_unique<tTableStyle>(wName.str());
        wStyle->SetShowRowStripes(true);
        wStyle->BuildFromElementCss(wFormatApi, wElements);
        m_Styles[wName.str()] = std::move(wStyle);
    }

    void tTableStyleContainer::InitBuiltins() {
        // Built-ins are registered lazily via EnsureBuiltinStyle — compiling all
        // 60 styles upfront IncCell's hundreds of pool entries that no sheet cell
        // owns (checkfo: Real>0 Check:0) and can collide with imported cell CSS.
        m_BuiltinsInitialized = true;
    }

    void tTableStyleContainer::EnsureBuiltins() {
        InitBuiltins();
    }

    tTableStyle* tTableStyleContainer::EnsureBuiltinStyle(const tString& sName) {
        if (sName.empty()) {
            return nullptr;
        }
        tTableStyle* wExisting = Find(sName);
        if (wExisting != nullptr) {
            return wExisting;
        }
        BuiltinStyleFamily wFamily = BuiltinStyleFamily::None;
        tInt wNumber = 0;
        if (!ParseBuiltinStyleName(sName, wFamily, wNumber)) {
            return nullptr;
        }
        if (wFamily == BuiltinStyleFamily::Medium && wNumber >= 1 && wNumber <= 28) {
            RegisterBuiltinMedium(wNumber);
        } else if (wFamily == BuiltinStyleFamily::Light && wNumber >= 1 && wNumber <= 21) {
            RegisterBuiltinLight(wNumber);
        } else if (wFamily == BuiltinStyleFamily::Dark && wNumber >= 1 && wNumber <= 11) {
            RegisterBuiltinDark(wNumber);
        } else {
            return nullptr;
        }
        return Find(sName);
    }

    tTableStyle* tTableStyleContainer::Find(const tString& sName) {
        if (sName.empty()) {
            return nullptr;
        }
        auto wIt = m_Styles.find(sName);
        if (wIt == m_Styles.end()) {
            return nullptr;
        }
        return wIt->second.get();
    }

    const tTableStyle* tTableStyleContainer::Find(const tString& sName) const {
        if (sName.empty()) {
            return nullptr;
        }
        auto wIt = m_Styles.find(sName);
        if (wIt == m_Styles.end()) {
            return nullptr;
        }
        return wIt->second.get();
    }

    tTableStyle* tTableStyleContainer::RegisterFromElementCss(
        const tString& sName,
        const std::map<tString, tString>& sElementCss) {
        if (sName.empty() || m_WorkBook == nullptr) {
            return nullptr;
        }
        tFormatApi* wFormatApi = m_WorkBook->FormatApi();
        if (wFormatApi == nullptr) {
            return nullptr;
        }
        tTableStyle* wExisting = Find(sName);
        if (wExisting != nullptr) {
            wExisting->BuildFromElementCss(wFormatApi, sElementCss);
            return wExisting;
        }
        auto wStyle = std::make_unique<tTableStyle>(sName);
        wStyle->BuildFromElementCss(wFormatApi, sElementCss);
        tTableStyle* wRaw = wStyle.get();
        m_Styles[sName] = std::move(wStyle);
        return wRaw;
    }

    tTableStyle* tTableStyleContainer::RegisterFromRangeData(
        const tString& sFallbackName,
        const tRangeData* sRangeData) {
        if (sRangeData == nullptr || !sRangeData->HasTableStyleElementCss()) {
            return nullptr;
        }
        tString wName = sRangeData->TableStyleName();
        if (wName.empty()) {
            wName = sFallbackName;
        }
        if (wName.empty()) {
            return nullptr;
        }
        tFormatApi* wFormatApi = m_WorkBook != nullptr ? m_WorkBook->FormatApi() : nullptr;
        if (wFormatApi == nullptr) {
            return nullptr;
        }
        tTableStyle* wExisting = Find(wName);
        std::map<tString, tString> wElements;
        for (const auto& wEntry : sRangeData->TableStyleElementCss()) {
            wElements[wEntry.first] = wEntry.second;
        }
        if (wExisting != nullptr) {
            wExisting->BuildFromElementCss(wFormatApi, wElements);
            wExisting->SetShowRowStripes(sRangeData->TableShowRowStripes());
            wExisting->SetShowColumnStripes(sRangeData->TableShowColumnStripes());
            wExisting->SetShowFirstColumn(sRangeData->TableShowFirstColumn());
            wExisting->SetShowLastColumn(sRangeData->TableShowLastColumn());
            return wExisting;
        }
        tTableStyle* wStyle = RegisterFromElementCss(wName, wElements);
        if (wStyle != nullptr) {
            wStyle->SetShowRowStripes(sRangeData->TableShowRowStripes());
            wStyle->SetShowColumnStripes(sRangeData->TableShowColumnStripes());
            wStyle->SetShowFirstColumn(sRangeData->TableShowFirstColumn());
            wStyle->SetShowLastColumn(sRangeData->TableShowLastColumn());
        }
        return wStyle;
    }

    tFormatRef tTableStyleContainer::OverlayForCell(tSheet* sSheet,
                                                    tIndex sRow,
                                                    tIndex sCol) {
        std::vector<tFormatRef> wLayers;
        AppendOverlayFormats(sSheet, sRow, sCol, wLayers);
        return wLayers.empty() ? 0 : wLayers.front();
    }

    void tTableStyleContainer::AppendOverlayFormats(tSheet* sSheet,
                                                    tIndex sRow,
                                                    tIndex sCol,
                                                    std::vector<tFormatRef>& sOut) {
        if (sSheet == nullptr || m_WorkBook == nullptr) {
            return;
        }
        EnsureBuiltins();

        tString wTableName;
        tRange* wRange = nullptr;
        std::tie(wTableName, wRange) = sSheet->FindRangeDataCovered(sRow, sCol);
        if (wRange == nullptr || !wRange->IsData() || wTableName.empty()) {
            return;
        }

        tRangeData* wRangeData = m_WorkBook->RangeData(wTableName);
        if (wRangeData == nullptr) {
            return;
        }

        tTableStyle* wStyle = nullptr;
        const tString wStyleName = wRangeData->TableStyleName();
        const tBool wBuiltinName = IsBuiltinExcelTableStyleName(wStyleName);
        if (wRangeData->HasTableStyleElementCss() && !wBuiltinName) {
            wStyle = RegisterFromRangeData(wTableName, wRangeData);
        }
        if (wStyle == nullptr && !wStyleName.empty()) {
            wStyle = Find(wStyleName);
        }
        if (wStyle == nullptr && wBuiltinName) {
            wStyle = EnsureBuiltinStyle(wStyleName);
        }
        if (wStyle == nullptr) {
            return;
        }

        wStyle->AppendOverlayFormats(
            m_WorkBook->FormatApi(), sRow, sCol, wRange, wRangeData, sOut);
    }

    tString tTableStyleContainer::ElementCssString(const tString& sName,
                                                   const tString& sType) const {
        if (sName.empty() || sType.empty()) {
            return "";
        }
        const tTableStyle* wStyle = Find(sName);
        if (wStyle != nullptr) {
            tFormatRef wRef = wStyle->ElementFormat(sType);
            if (wRef == 0 && sType == TableStyleElement::kTotalRow) {
                wRef = wStyle->ElementFormat("totalsRow");
            }
            if (wRef != 0 && m_WorkBook != nullptr) {
                return m_WorkBook->CellFormat(wRef);
            }
        }
        // Built-in CSS without compiling FormatRef (no IncCell / checkfo noise).
        tString wCss = BuildBuiltinTableStyleElementCss(sName, sType);
        if (!wCss.empty()) {
            return wCss;
        }
        if (sType == TableStyleElement::kTotalRow) {
            return BuildBuiltinTableStyleElementCss(sName, "totalsRow");
        }
        return "";
    }

    tString tTableStyleContainer::OverlayCssString(tSheet* sSheet,
                                                   tIndex sRow,
                                                   tIndex sCol) {
        if (m_WorkBook == nullptr || m_WorkBook->FormatApi() == nullptr) {
            return "";
        }
        std::vector<tFormatRef> wLayers;
        AppendOverlayFormats(sSheet, sRow, sCol, wLayers);
        if (wLayers.empty()) {
            return "";
        }
        if (wLayers.size() == 1) {
            return m_WorkBook->CellFormat(wLayers.front());
        }
        return m_WorkBook->CellFormat(&wLayers);
    }

    void tTableStyleContainer::Clear() {
        // ReleaseFormats runs in each style; destructor is a safety net if erase skips Clear.
        tFormatApi* wFormatApi = m_WorkBook != nullptr ? m_WorkBook->FormatApi() : nullptr;
        for (auto& wEntry : m_Styles) {
            if (wEntry.second != nullptr) {
                wEntry.second->ReleaseFormats(wFormatApi);
            }
        }
        m_Styles.clear();
        m_BuiltinsInitialized = false;
    }

#ifdef checkfo
    void tTableStyleContainer::CheckFormat() {
        tFormatApi* wFormatApi = m_WorkBook != nullptr ? m_WorkBook->FormatApi() : nullptr;
        if (wFormatApi == nullptr) {
            return;
        }
        for (auto& wEntry : m_Styles) {
            if (wEntry.second != nullptr) {
                wEntry.second->IncCheckFormats(wFormatApi);
            }
        }
    }
#endif

} // namespace SkSpreadSheet
