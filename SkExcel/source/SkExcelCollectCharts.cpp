//=============================================================================
// SkExcelCollectCharts.cpp — collect Excel drawing charts for Sker import
//=============================================================================
#include "SkExcelPugiXMLReader.hpp"

#include <cstdlib>
#include <functional>
#include <map>
#include <sstream>

namespace {

tString ReadSeriesFormula(const pugi::xml_node& sSer, const tString& sCatOrVal, const tString& sRefKind) {
    pugi::xml_node wAxis = sSer.child(("c:" + sCatOrVal).c_str());
    if (!wAxis) {
        wAxis = sSer.child(sCatOrVal.c_str());
    }
    if (!wAxis) {
        return tString("");
    }
    pugi::xml_node wRef = wAxis.child(("c:" + sRefKind + "Ref").c_str());
    if (!wRef) {
        wRef = wAxis.child((sRefKind + "Ref").c_str());
    }
    if (!wRef) {
        return tString("");
    }
    pugi::xml_node wFormula = wRef.child("c:f");
    if (!wFormula) {
        wFormula = wRef.child("f");
    }
    if (!wFormula) {
        return tString("");
    }
    return tString(wFormula.text().as_string(""));
}

tString ReadChartTitleText(pugi::xml_node sChart) {
    if (!sChart) {
        return tString("");
    }
    pugi::xml_node wTitle = sChart.child("c:title");
    if (!wTitle) {
        wTitle = sChart.child("title");
    }
    if (!wTitle) {
        return tString("");
    }
    pugi::xml_node wRich = wTitle.child("c:tx").child("c:rich");
    if (!wRich) {
        wRich = wTitle.child("tx").child("rich");
    }
    if (wRich) {
        tString wText;
        for (pugi::xml_node wP = wRich.child("a:p"); wP; wP = wP.next_sibling("a:p")) {
            for (pugi::xml_node wR = wP.child("a:r"); wR; wR = wR.next_sibling("a:r")) {
                pugi::xml_node wT = wR.child("a:t");
                if (wT) {
                    wText += wT.text().as_string("");
                }
            }
        }
        return wText;
    }
    pugi::xml_node wFormula = wTitle.child("c:tx").child("c:strRef").child("c:f");
    if (!wFormula) {
        wFormula = wTitle.child("tx").child("strRef").child("f");
    }
    if (wFormula) {
        return tString(wFormula.text().as_string(""));
    }
    return tString("");
}

tBool ParseChartDocument(const pugi::xml_document& sChartDoc, tExcelChartEntry& ioEntry) {
    pugi::xml_node wChartSpace = sChartDoc.child("c:chartSpace");
    if (!wChartSpace) {
        wChartSpace = sChartDoc.child("chartSpace");
    }
    pugi::xml_node wChart = wChartSpace.child("c:chart");
    if (!wChart) {
        wChart = wChartSpace.child("chart");
    }
    pugi::xml_node wPlot = wChart.child("c:plotArea");
    if (!wPlot) {
        wPlot = wChart.child("plotArea");
    }
    if (!wPlot) {
        return false;
    }

    ioEntry.Title = ReadChartTitleText(wChart);

    struct tChartTypeMap {
        const char* XmlContainer;
        const char* XmlSeries;
        const char* SkerType;
    };
    static const tChartTypeMap wTypes[] = {
        {"c:barChart", "c:ser", "bar"},
        {"barChart", "ser", "bar"},
        {"c:lineChart", "c:ser", "line"},
        {"lineChart", "ser", "line"},
        {"c:areaChart", "c:ser", "area"},
        {"areaChart", "ser", "area"},
        {"c:pieChart", "c:ser", "pie"},
        {"pieChart", "ser", "pie"},
    };

    pugi::xml_node wSeriesContainer;
    const char* wSeriesTag = "c:ser";
    ioEntry.SkerChartType = "line";
    for (const tChartTypeMap& wMap : wTypes) {
        wSeriesContainer = wPlot.child(wMap.XmlContainer);
        if (!wSeriesContainer) {
            continue;
        }
        ioEntry.SkerChartType = wMap.SkerType;
        wSeriesTag = wMap.XmlSeries;
        break;
    }
    if (!wSeriesContainer) {
        return false;
    }

    ioEntry.BarDirection = "vertical";
    if (ioEntry.SkerChartType == "bar") {
        pugi::xml_node wBarDir = wSeriesContainer.child("c:barDir");
        if (!wBarDir) {
            wBarDir = wSeriesContainer.child("barDir");
        }
        const char* wBarDirVal = wBarDir.attribute("val").as_string("col");
        if (std::strcmp(wBarDirVal, "bar") == 0) {
            ioEntry.BarDirection = "horizontal";
        }
    }

    for (pugi::xml_node wSer = wSeriesContainer.child(wSeriesTag); wSer; wSer = wSer.next_sibling(wSeriesTag)) {
        tExcelChartSeriesEntry wSeries;
        wSeries.NameRef = ReadSeriesFormula(wSer, "tx", "str");
        wSeries.CategoryRef = ReadSeriesFormula(wSer, "cat", "str");
        wSeries.ValueRef = ReadSeriesFormula(wSer, "val", "num");
        if (wSeries.CategoryRef.empty() && wSeries.ValueRef.empty()) {
            continue;
        }
        ioEntry.Series.push_back(std::move(wSeries));
    }
    return !ioEntry.Series.empty();
}

tBool IsAnchorNodeNameCharts(const tString& sName) {
    return sName == "xdr:twoCellAnchor" || sName == "twoCellAnchor"
        || sName == "xdr:oneCellAnchor" || sName == "oneCellAnchor"
        || sName == "xdr:absoluteAnchor" || sName == "absoluteAnchor";
}

pugi::xml_node FindGraphicFrame(const pugi::xml_node& sAnchor) {
    pugi::xml_node wFrame = sAnchor.child("xdr:graphicFrame");
    if (!wFrame) {
        wFrame = sAnchor.child("graphicFrame");
    }
    return wFrame;
}

const char* FindChartRelId(const pugi::xml_node& sFrame) {
    if (!sFrame) {
        return nullptr;
    }
    pugi::xml_node wGraphic = sFrame.child("a:graphic");
    if (!wGraphic) {
        wGraphic = sFrame.child("graphic");
    }
    if (!wGraphic) {
        return nullptr;
    }
    pugi::xml_node wGd = wGraphic.child("a:graphicData");
    if (!wGd) {
        wGd = wGraphic.child("graphicData");
    }
    if (!wGd) {
        return nullptr;
    }
    pugi::xml_node wChart = wGd.child("c:chart");
    if (!wChart) {
        wChart = wGd.child("chart");
    }
    if (!wChart) {
        return nullptr;
    }
    const char* wRid = wChart.attribute("r:id").as_string("");
    if (wRid == nullptr || *wRid == '\0') {
        wRid = wChart.attribute("id").as_string("");
    }
    return (wRid != nullptr && *wRid != '\0') ? wRid : nullptr;
}

} // namespace

std::vector<tExcelChartEntry> tExcelPugiXMLReader::CollectCharts() const {
    std::vector<tExcelChartEntry> wOut;
    if (m_UnzippedFiles.empty()) {
        return wOut;
    }

    auto wColLetters = [](tInt wC) {
        tString w;
        tInt c = wC;
        do {
            const tInt r = c % 26;
            w.insert(w.begin(), static_cast<char>('A' + r));
            c = c / 26 - 1;
        } while (c >= 0);
        return w;
    };

    auto wEmuToPx = [](long long wEmu, tDouble wDpi = 96.0) -> tInt {
        const tDouble wEmusPerInch = 914400.0;
        return static_cast<tInt>(std::round(static_cast<tDouble>(wEmu) / wEmusPerInch * wDpi));
    };

    auto wResolveTarget = [](const tString& target) -> tString {
        if (target.rfind("../", 0) == 0) {
            return tString("xl/") + target.substr(3);
        }
        if (target.rfind("xl/", 0) == 0) {
            return target;
        }
        return target;
    };

    for (tInt wS = 1;; ++wS) {
        const tString wSheetFile = "xl/worksheets/sheet" + std::to_string(wS) + ".xml";
        auto wItSheet = m_UnzippedFiles.find(wSheetFile);
        if (wItSheet == m_UnzippedFiles.end()) {
            break;
        }

        const tString wSheetName = (wS - 1) >= 0 && (wS - 1) < static_cast<tInt>(m_WorksheetNames.size())
            ? m_WorksheetNames[static_cast<tSize>(wS - 1)]
            : ("sheet" + std::to_string(wS));

        const tString wRelsPath = "xl/worksheets/_rels/sheet" + std::to_string(wS) + ".xml.rels";
        auto wItRels = m_UnzippedFiles.find(wRelsPath);
        if (wItRels == m_UnzippedFiles.end()) {
            continue;
        }

        pugi::xml_document wRelDoc;
        if (!wRelDoc.load_buffer(wItRels->second.content.data(), wItRels->second.content.size())) {
            continue;
        }

        std::vector<tString> wDrawingTargets;
        for (pugi::xml_node wRel : wRelDoc.child("Relationships").children("Relationship")) {
            const tString wType = wRel.attribute("Type").as_string("");
            if (wType.find("/drawing") == tString::npos) {
                continue;
            }
            tString wTarget = wResolveTarget(wRel.attribute("Target").as_string(""));
            if (wTarget.find("xl/drawings/") == tString::npos) {
                const tSize p = wTarget.find("drawings/");
                if (p != tString::npos) {
                    wTarget = tString("xl/") + wTarget.substr(p);
                }
            }
            wDrawingTargets.push_back(wTarget);
        }

        for (const tString& wDrawPath : wDrawingTargets) {
            auto wItDrawXml = m_UnzippedFiles.find(wDrawPath);
            if (wItDrawXml == m_UnzippedFiles.end()) {
                continue;
            }

            pugi::xml_document wDrawDoc;
            if (!wDrawDoc.load_buffer(wItDrawXml->second.content.data(), wItDrawXml->second.content.size())) {
                continue;
            }

            tString wDrawRelsPath = wDrawPath + ".rels";
            if (wDrawRelsPath.find("xl/drawings/") != tString::npos) {
                wDrawRelsPath.insert(
                    wDrawRelsPath.find("xl/drawings/") + tString("xl/drawings/").size(),
                    "_rels/");
            }
            auto wItDrawRels = m_UnzippedFiles.find(wDrawRelsPath);
            if (wItDrawRels == m_UnzippedFiles.end()) {
                continue;
            }

            pugi::xml_document wDrawRelDoc;
            if (!wDrawRelDoc.load_buffer(wItDrawRels->second.content.data(), wItDrawRels->second.content.size())) {
                continue;
            }

            std::map<tString, tString> wRelIdToChartPath;
            for (pugi::xml_node wRel : wDrawRelDoc.child("Relationships").children("Relationship")) {
                const tString wType = wRel.attribute("Type").as_string("");
                if (wType.find("/chart") == tString::npos) {
                    continue;
                }
                tString wTarget = wRel.attribute("Target").as_string("");
                if (wTarget.rfind("../", 0) == 0) {
                    wTarget = tString("xl/") + wTarget.substr(3);
                } else if (wTarget.find("xl/charts/") == tString::npos) {
                    const tSize p = wTarget.find("charts/");
                    if (p != tString::npos) {
                        wTarget = tString("xl/") + wTarget.substr(p);
                    }
                }
                wRelIdToChartPath[wRel.attribute("Id").as_string("")] = wTarget;
            }

            pugi::xml_node wDrawRoot = wDrawDoc.child("xdr:wsDr");
            if (!wDrawRoot) {
                wDrawRoot = wDrawDoc.child("wsDr");
            }
            if (!wDrawRoot) {
                wDrawRoot = wDrawDoc.document_element();
            }

            for (pugi::xml_node wAnchor : wDrawRoot.children()) {
                const tString wAnchorName = wAnchor.name();
                if (!IsAnchorNodeNameCharts(wAnchorName)) {
                    continue;
                }

                pugi::xml_node wFrame = FindGraphicFrame(wAnchor);
                const char* wRelId = FindChartRelId(wFrame);
                if (wRelId == nullptr) {
                    continue;
                }

                auto wChartIt = wRelIdToChartPath.find(wRelId);
                if (wChartIt == wRelIdToChartPath.end()) {
                    continue;
                }
                auto wItChartFile = m_UnzippedFiles.find(wChartIt->second);
                if (wItChartFile == m_UnzippedFiles.end()) {
                    continue;
                }

                pugi::xml_document wChartDoc;
                if (!wChartDoc.load_buffer(wItChartFile->second.content.data(), wItChartFile->second.content.size())) {
                    continue;
                }

                tExcelChartEntry wEntry;
                wEntry.SheetName = wSheetName;
                wEntry.AnchorType = wAnchorName;
                if (!ParseChartDocument(wChartDoc, wEntry)) {
                    continue;
                }

                pugi::xml_node wNv = wFrame.child("xdr:nvGraphicFramePr");
                if (!wNv) {
                    wNv = wFrame.child("nvGraphicFramePr");
                }
                if (wNv) {
                    pugi::xml_node wCNvPr = wNv.child("xdr:cNvPr");
                    if (!wCNvPr) {
                        wCNvPr = wNv.child("cNvPr");
                    }
                    if (wCNvPr) {
                        wEntry.Name = wCNvPr.attribute("name").as_string("");
                    }
                }

                tString wPosition = "unknown";
                tInt wWidthPx = 0;
                tInt wHeightPx = 0;
                tInt wAnchorRow = 0;
                tInt wAnchorCol = 0;
                tDouble wDiffX = 0.0;
                tDouble wDiffY = 0.0;
                tInt wToAnchorRow = 0;
                tInt wToAnchorCol = 0;
                tDouble wToDiffX = 0.0;
                tDouble wToDiffY = 0.0;

                auto wApplyFromNode = [&](const pugi::xml_node& wFrom) {
                    if (!wFrom) {
                        return;
                    }
                    tInt wColFrom = wFrom.child("xdr:col").text().as_int(-1);
                    if (wColFrom < 0) {
                        wColFrom = wFrom.child("col").text().as_int(-1);
                    }
                    tInt wRowFrom = wFrom.child("xdr:row").text().as_int(-1);
                    if (wRowFrom < 0) {
                        wRowFrom = wFrom.child("row").text().as_int(-1);
                    }
                    if (wColFrom < 0 || wRowFrom < 0) {
                        return;
                    }
                    wPosition = wColLetters(wColFrom) + std::to_string(wRowFrom + 1);
                    wAnchorRow = wRowFrom + 1;
                    wAnchorCol = wColFrom + 1;
                    long long wColOff = wFrom.child("xdr:colOff").text().as_llong(0);
                    if (wColOff == 0) {
                        wColOff = wFrom.child("colOff").text().as_llong(0);
                    }
                    long long wRowOff = wFrom.child("xdr:rowOff").text().as_llong(0);
                    if (wRowOff == 0) {
                        wRowOff = wFrom.child("rowOff").text().as_llong(0);
                    }
                    wDiffX = static_cast<tDouble>(wEmuToPx(wColOff));
                    wDiffY = static_cast<tDouble>(wEmuToPx(wRowOff));
                };

                auto wApplyToNode = [&](const pugi::xml_node& wTo) {
                    if (!wTo) {
                        return;
                    }
                    tInt wColTo = wTo.child("xdr:col").text().as_int(-1);
                    if (wColTo < 0) {
                        wColTo = wTo.child("col").text().as_int(-1);
                    }
                    tInt wRowTo = wTo.child("xdr:row").text().as_int(-1);
                    if (wRowTo < 0) {
                        wRowTo = wTo.child("row").text().as_int(-1);
                    }
                    if (wColTo < 0 || wRowTo < 0) {
                        return;
                    }
                    wToAnchorRow = wRowTo + 1;
                    wToAnchorCol = wColTo + 1;
                    long long wColOff = wTo.child("xdr:colOff").text().as_llong(0);
                    if (wColOff == 0) {
                        wColOff = wTo.child("colOff").text().as_llong(0);
                    }
                    long long wRowOff = wTo.child("xdr:rowOff").text().as_llong(0);
                    if (wRowOff == 0) {
                        wRowOff = wTo.child("rowOff").text().as_llong(0);
                    }
                    wToDiffX = static_cast<tDouble>(wEmuToPx(wColOff));
                    wToDiffY = static_cast<tDouble>(wEmuToPx(wRowOff));
                };

                pugi::xml_node wXfrm = wFrame.child("xdr:xfrm");
                if (!wXfrm) {
                    wXfrm = wFrame.child("xfrm");
                }
                if (wXfrm) {
                    pugi::xml_node wExt = wXfrm.child("a:ext");
                    if (!wExt) {
                        wExt = wXfrm.child("ext");
                    }
                    if (wExt) {
                        wWidthPx = wEmuToPx(wExt.attribute("cx").as_llong(0));
                        wHeightPx = wEmuToPx(wExt.attribute("cy").as_llong(0));
                    }
                }

                if (wAnchorName == "xdr:twoCellAnchor" || wAnchorName == "twoCellAnchor") {
                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                    if (!wFrom) {
                        wFrom = wAnchor.child("from");
                    }
                    wApplyFromNode(wFrom);
                    pugi::xml_node wTo = wAnchor.child("xdr:to");
                    if (!wTo) {
                        wTo = wAnchor.child("to");
                    }
                    wApplyToNode(wTo);
                } else if (wAnchorName == "xdr:oneCellAnchor" || wAnchorName == "oneCellAnchor") {
                    pugi::xml_node wFrom = wAnchor.child("xdr:from");
                    if (!wFrom) {
                        wFrom = wAnchor.child("from");
                    }
                    wApplyFromNode(wFrom);
                    pugi::xml_node wExt = wAnchor.child("xdr:ext");
                    if (!wExt) {
                        wExt = wAnchor.child("ext");
                    }
                    if (wExt && wWidthPx <= 0) {
                        wWidthPx = wEmuToPx(wExt.attribute("cx").as_llong(0));
                        wHeightPx = wEmuToPx(wExt.attribute("cy").as_llong(0));
                    }
                } else if (wAnchorName == "xdr:absoluteAnchor" || wAnchorName == "absoluteAnchor") {
                    pugi::xml_node wPosNode = wAnchor.child("xdr:pos");
                    if (!wPosNode) {
                        wPosNode = wAnchor.child("pos");
                    }
                    if (wPosNode) {
                        wAnchorRow = 1;
                        wAnchorCol = 1;
                        wPosition = "A1";
                        wDiffX = static_cast<tDouble>(wEmuToPx(wPosNode.attribute("x").as_llong(0)));
                        wDiffY = static_cast<tDouble>(wEmuToPx(wPosNode.attribute("y").as_llong(0)));
                    }
                }

                wEntry.Position = wPosition;
                wEntry.WidthPx = wWidthPx;
                wEntry.HeightPx = wHeightPx;
                wEntry.AnchorRow = wAnchorRow;
                wEntry.AnchorCol = wAnchorCol;
                wEntry.DiffX = wDiffX;
                wEntry.DiffY = wDiffY;
                wEntry.ToAnchorRow = wToAnchorRow;
                wEntry.ToAnchorCol = wToAnchorCol;
                wEntry.ToDiffX = wToDiffX;
                wEntry.ToDiffY = wToDiffY;
                wOut.push_back(std::move(wEntry));
            }
        }
    }

    return wOut;
}
