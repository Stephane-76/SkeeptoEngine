//=============================================================================
// SkPrintParameters.cpp
//=============================================================================
#include "../include/SkPrintParameters.hpp"
#include "../include/SkJsonKey.hpp"

#include <cmath>
#include <limits>

namespace SkSpreadSheet {

namespace {

const tPrintParameters& DefaultSnapshot()
{
    static const tPrintParameters kDefaults;
    return kDefaults;
}

//! Margins may be assigned from computations; compare with a tiny tolerance.
inline bool MarginsEqual(tDouble a, tDouble b)
{
    return std::abs(a - b) <= 1.0e-9;
}

tBool JsonTryInt(const rapidjson::Value& sValue, tInt* out)
{
    if (sValue.IsInt()) {
        *out = sValue.GetInt();
        return true;
    }
    if (sValue.IsUint() && sValue.GetUint() <= static_cast<unsigned>(std::numeric_limits<tInt>::max())) {
        *out = static_cast<tInt>(sValue.GetUint());
        return true;
    }
    return false;
}

//! Accepts int/uint or whole JSON numbers (some exporters use doubles for integer fields).
tBool JsonTryIntCoerce(const rapidjson::Value& sValue, tInt* out)
{
    if (JsonTryInt(sValue, out)) {
        return true;
    }
    if (sValue.IsNumber() && sValue.IsLosslessDouble()) {
        const double wD = sValue.GetDouble();
        if (wD < static_cast<double>(std::numeric_limits<tInt>::min()) ||
            wD > static_cast<double>(std::numeric_limits<tInt>::max())) {
            return false;
        }
        if (std::floor(wD) != wD) {
            return false;
        }
        *out = static_cast<tInt>(wD);
        return true;
    }
    return false;
}

//! OOXML DPI is typically 72–2400; reject garbage (overflow, bad imports) and fall back to Excel default 600.
void SanitizeDpi(tInt& ioDpi)
{
    constexpr tInt kDefault = 600;
    constexpr tInt kMin = 72;
    constexpr tInt kMax = 9600;
    if (ioDpi < kMin || ioDpi > kMax) {
        ioDpi = kDefault;
    }
}

tBool JsonTryDouble(const rapidjson::Value& sValue, tDouble* out)
{
    if (!sValue.IsNumber() || !sValue.IsLosslessDouble()) {
        return false;
    }
    *out = sValue.GetDouble();
    return true;
}

tBool ReadLimitedEnumInt(const rapidjson::Value& sValue, tInt sMaxInclusive, tInt* out)
{
    tInt v = 0;
    if (!JsonTryInt(sValue, &v) || v < 0 || v > sMaxInclusive) {
        return false;
    }
    *out = v;
    return true;
}

} // namespace

tPrintParameters::tPrintParameters()
    : m_PaperSize(9)
    , m_Orientation(tPrintOrientation::Portrait)
    , m_Scale(100)
    , m_FitToPage(true)
    , m_FitToWidthPages(1)
    , m_FitToHeightPages(1)
    , m_PageOrder(tPrintPageOrder::DownThenOver)
    , m_BlackAndWhite(false)
    , m_Draft(false)
    , m_UsePrinterDefaults(true)
    , m_HorizontalDpi(600)
    , m_VerticalDpi(600)
    , m_Copies(1)
    , m_FirstPageNumber(0)
    , m_MarginLeft(0.7)
    , m_MarginRight(0.7)
    , m_MarginTop(0.75)
    , m_MarginBottom(0.75)
    , m_MarginHeader(0.3)
    , m_MarginFooter(0.3)
    , m_PrintGridLines(false)
    , m_PrintHeadings(false)
    , m_HorizontalCentered(false)
    , m_VerticalCentered(false)
    , m_AutoPageBreaks(true)
{
}

void tPrintParameters::JsonWrite(Writer<StringBuffer>* sWriter, tPrintJsonScope sScope) const
{
    if (sWriter == nullptr) {
        return;
    }
    const tPrintParameters& d = DefaultSnapshot();
    const tBool wWorkBook = (sScope == tPrintJsonScope::All) || (sScope == tPrintJsonScope::WorkBook);
    const tBool wSheet = (sScope == tPrintJsonScope::All) || (sScope == tPrintJsonScope::Sheet);

    sWriter->StartObject();

    if (wWorkBook && m_PaperSize != d.m_PaperSize) {
        sWriter->Key(kJsonKeyPaperSize);
        sWriter->Int(m_PaperSize);
    }
    if (wSheet && m_Orientation != d.m_Orientation) {
        sWriter->Key(kJsonKeyOrientation);
        sWriter->Int(static_cast<int>(m_Orientation));
    }
    if (wWorkBook && m_Scale != d.m_Scale) {
        sWriter->Key(kJsonKeyScale);
        sWriter->Int(m_Scale);
    }
    if (wSheet && m_FitToPage != d.m_FitToPage) {
        sWriter->Key(kJsonKeyFitToPage);
        sWriter->Bool(m_FitToPage);
    }
    if (wWorkBook && m_FitToWidthPages != d.m_FitToWidthPages) {
        sWriter->Key(kJsonKeyFitToWidthPages);
        sWriter->Int(m_FitToWidthPages);
    }
    if (wWorkBook && m_FitToHeightPages != d.m_FitToHeightPages) {
        sWriter->Key(kJsonKeyFitToHeightPages);
        sWriter->Int(m_FitToHeightPages);
    }
    if (wWorkBook && m_PageOrder != d.m_PageOrder) {
        sWriter->Key(kJsonKeyPageOrder);
        sWriter->Int(static_cast<int>(m_PageOrder));
    }
    if (wWorkBook && m_BlackAndWhite != d.m_BlackAndWhite) {
        sWriter->Key(kJsonKeyBlackAndWhite);
        sWriter->Bool(m_BlackAndWhite);
    }
    if (wWorkBook && m_Draft != d.m_Draft) {
        sWriter->Key(kJsonKeyDraft);
        sWriter->Bool(m_Draft);
    }
    if (wWorkBook && m_UsePrinterDefaults != d.m_UsePrinterDefaults) {
        sWriter->Key(kJsonKeyUsePrinterDefaults);
        sWriter->Bool(m_UsePrinterDefaults);
    }
    if (wWorkBook && m_HorizontalDpi != d.m_HorizontalDpi) {
        sWriter->Key(kJsonKeyHorizontalDpi);
        sWriter->Int(m_HorizontalDpi);
    }
    if (wWorkBook && m_VerticalDpi != d.m_VerticalDpi) {
        sWriter->Key(kJsonKeyVerticalDpi);
        sWriter->Int(m_VerticalDpi);
    }
    if (wWorkBook && m_Copies != d.m_Copies) {
        sWriter->Key(kJsonKeyCopies);
        sWriter->Int(m_Copies);
    }
    if (wWorkBook && m_FirstPageNumber != d.m_FirstPageNumber) {
        sWriter->Key(kJsonKeyFirstPageNumber);
        sWriter->Int(m_FirstPageNumber);
    }
    if (wWorkBook && !MarginsEqual(m_MarginLeft, d.m_MarginLeft)) {
        sWriter->Key(kJsonKeyMarginLeft);
        sWriter->Double(m_MarginLeft);
    }
    if (wWorkBook && !MarginsEqual(m_MarginRight, d.m_MarginRight)) {
        sWriter->Key(kJsonKeyMarginRight);
        sWriter->Double(m_MarginRight);
    }
    if (wWorkBook && !MarginsEqual(m_MarginTop, d.m_MarginTop)) {
        sWriter->Key(kJsonKeyMarginTop);
        sWriter->Double(m_MarginTop);
    }
    if (wWorkBook && !MarginsEqual(m_MarginBottom, d.m_MarginBottom)) {
        sWriter->Key(kJsonKeyMarginBottom);
        sWriter->Double(m_MarginBottom);
    }
    if (wWorkBook && !MarginsEqual(m_MarginHeader, d.m_MarginHeader)) {
        sWriter->Key(kJsonKeyMarginHeader);
        sWriter->Double(m_MarginHeader);
    }
    if (wWorkBook && !MarginsEqual(m_MarginFooter, d.m_MarginFooter)) {
        sWriter->Key(kJsonKeyMarginFooter);
        sWriter->Double(m_MarginFooter);
    }
    if (wWorkBook && m_PrintGridLines != d.m_PrintGridLines) {
        sWriter->Key(kJsonKeyPrintGridLines);
        sWriter->Bool(m_PrintGridLines);
    }
    if (wWorkBook && m_PrintHeadings != d.m_PrintHeadings) {
        sWriter->Key(kJsonKeyPrintHeadings);
        sWriter->Bool(m_PrintHeadings);
    }
    if (wWorkBook && m_HorizontalCentered != d.m_HorizontalCentered) {
        sWriter->Key(kJsonKeyHorizontalCentered);
        sWriter->Bool(m_HorizontalCentered);
    }
    if (wWorkBook && m_VerticalCentered != d.m_VerticalCentered) {
        sWriter->Key(kJsonKeyVerticalCentered);
        sWriter->Bool(m_VerticalCentered);
    }
    if (wWorkBook && m_AutoPageBreaks != d.m_AutoPageBreaks) {
        sWriter->Key(kJsonKeyAutoPageBreaks);
        sWriter->Bool(m_AutoPageBreaks);
    }

    sWriter->EndObject();
}

tString tPrintParameters::JsonString(tPrintJsonScope sScope) const
{
    StringBuffer wBuffer;
    Writer<StringBuffer> wWriter(wBuffer);
    JsonWrite(&wWriter, sScope);
    return tString(wBuffer.GetString());
}

tBool tPrintParameters::JsonRead(const rapidjson::Value& sValue)
{
    if (!sValue.IsObject()) {
        return false;
    }
    *this = DefaultSnapshot();

    tInt wi = 0;
    tDouble wd = 0.0;

    if (sValue.HasMember(kJsonKeyPaperSize)) {
        const rapidjson::Value& v = sValue[kJsonKeyPaperSize];
        if (!JsonTryInt(v, &m_PaperSize)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyOrientation)) {
        if (!ReadLimitedEnumInt(sValue[kJsonKeyOrientation], 1, &wi)) {
            return false;
        }
        m_Orientation = static_cast<tPrintOrientation>(wi);
    }
    if (sValue.HasMember(kJsonKeyScale)) {
        if (!JsonTryInt(sValue[kJsonKeyScale], &m_Scale)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyFitToPage)) {
        const rapidjson::Value& v = sValue[kJsonKeyFitToPage];
        if (!v.IsBool()) {
            return false;
        }
        m_FitToPage = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyFitToWidthPages)) {
        if (!JsonTryInt(sValue[kJsonKeyFitToWidthPages], &m_FitToWidthPages)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyFitToHeightPages)) {
        if (!JsonTryInt(sValue[kJsonKeyFitToHeightPages], &m_FitToHeightPages)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyPageOrder)) {
        if (!ReadLimitedEnumInt(sValue[kJsonKeyPageOrder], 1, &wi)) {
            return false;
        }
        m_PageOrder = static_cast<tPrintPageOrder>(wi);
    }
    if (sValue.HasMember(kJsonKeyBlackAndWhite)) {
        const rapidjson::Value& v = sValue[kJsonKeyBlackAndWhite];
        if (!v.IsBool()) {
            return false;
        }
        m_BlackAndWhite = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyDraft)) {
        const rapidjson::Value& v = sValue[kJsonKeyDraft];
        if (!v.IsBool()) {
            return false;
        }
        m_Draft = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyUsePrinterDefaults)) {
        const rapidjson::Value& v = sValue[kJsonKeyUsePrinterDefaults];
        if (!v.IsBool()) {
            return false;
        }
        m_UsePrinterDefaults = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyHorizontalDpi)) {
        tInt wTmp = m_HorizontalDpi;
        if (JsonTryIntCoerce(sValue[kJsonKeyHorizontalDpi], &wTmp)) {
            m_HorizontalDpi = wTmp;
            SanitizeDpi(m_HorizontalDpi);
        }
    }
    if (sValue.HasMember(kJsonKeyVerticalDpi)) {
        tInt wTmp = m_VerticalDpi;
        if (JsonTryIntCoerce(sValue[kJsonKeyVerticalDpi], &wTmp)) {
            m_VerticalDpi = wTmp;
            SanitizeDpi(m_VerticalDpi);
        }
    }
    if (sValue.HasMember(kJsonKeyCopies)) {
        if (!JsonTryInt(sValue[kJsonKeyCopies], &m_Copies)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyFirstPageNumber)) {
        if (!JsonTryInt(sValue[kJsonKeyFirstPageNumber], &m_FirstPageNumber)) {
            return false;
        }
    }
    if (sValue.HasMember(kJsonKeyMarginLeft)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginLeft], &wd)) {
            return false;
        }
        m_MarginLeft = wd;
    }
    if (sValue.HasMember(kJsonKeyMarginRight)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginRight], &wd)) {
            return false;
        }
        m_MarginRight = wd;
    }
    if (sValue.HasMember(kJsonKeyMarginTop)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginTop], &wd)) {
            return false;
        }
        m_MarginTop = wd;
    }
    if (sValue.HasMember(kJsonKeyMarginBottom)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginBottom], &wd)) {
            return false;
        }
        m_MarginBottom = wd;
    }
    if (sValue.HasMember(kJsonKeyMarginHeader)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginHeader], &wd)) {
            return false;
        }
        m_MarginHeader = wd;
    }
    if (sValue.HasMember(kJsonKeyMarginFooter)) {
        if (!JsonTryDouble(sValue[kJsonKeyMarginFooter], &wd)) {
            return false;
        }
        m_MarginFooter = wd;
    }
    if (sValue.HasMember(kJsonKeyPrintGridLines)) {
        const rapidjson::Value& v = sValue[kJsonKeyPrintGridLines];
        if (!v.IsBool()) {
            return false;
        }
        m_PrintGridLines = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyPrintHeadings)) {
        const rapidjson::Value& v = sValue[kJsonKeyPrintHeadings];
        if (!v.IsBool()) {
            return false;
        }
        m_PrintHeadings = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyHorizontalCentered)) {
        const rapidjson::Value& v = sValue[kJsonKeyHorizontalCentered];
        if (!v.IsBool()) {
            return false;
        }
        m_HorizontalCentered = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyVerticalCentered)) {
        const rapidjson::Value& v = sValue[kJsonKeyVerticalCentered];
        if (!v.IsBool()) {
            return false;
        }
        m_VerticalCentered = v.GetBool();
    }
    if (sValue.HasMember(kJsonKeyAutoPageBreaks)) {
        const rapidjson::Value& v = sValue[kJsonKeyAutoPageBreaks];
        if (!v.IsBool()) {
            return false;
        }
        m_AutoPageBreaks = v.GetBool();
    }

    return true;
}

tBool tPrintParameters::JsonParse(const tString& sJson)
{
    return JsonParse(sJson.data(), sJson.size());
}

tBool tPrintParameters::JsonParse(const tChar* sUtf8, size_t sLength)
{
    if (sUtf8 == nullptr) {
        return false;
    }
    Document wDoc;
    wDoc.Parse(sUtf8, sLength);
    if (wDoc.HasParseError()) {
        return false;
    }
    return JsonRead(wDoc);
}

void tPrintParameters::CopyWorkBookFieldsFrom(const tPrintParameters& sSrc)
{
    const tPrintOrientation wOrientation = m_Orientation;
    const tBool wFitToPage = m_FitToPage;
    *this = sSrc;
    m_Orientation = wOrientation;
    m_FitToPage = wFitToPage;
}

void tPrintParameters::ClearSheetFields()
{
    const tPrintParameters& d = DefaultSnapshot();
    m_Orientation = d.m_Orientation;
    m_FitToPage = d.m_FitToPage;
}

tBool tPrintParameters::WorkBookFieldsAreDefault() const
{
    const tPrintParameters& d = DefaultSnapshot();
    return m_PaperSize == d.m_PaperSize
        && m_Scale == d.m_Scale
        && m_FitToWidthPages == d.m_FitToWidthPages
        && m_FitToHeightPages == d.m_FitToHeightPages
        && m_PageOrder == d.m_PageOrder
        && m_BlackAndWhite == d.m_BlackAndWhite
        && m_Draft == d.m_Draft
        && m_UsePrinterDefaults == d.m_UsePrinterDefaults
        && m_HorizontalDpi == d.m_HorizontalDpi
        && m_VerticalDpi == d.m_VerticalDpi
        && m_Copies == d.m_Copies
        && m_FirstPageNumber == d.m_FirstPageNumber
        && MarginsEqual(m_MarginLeft, d.m_MarginLeft)
        && MarginsEqual(m_MarginRight, d.m_MarginRight)
        && MarginsEqual(m_MarginTop, d.m_MarginTop)
        && MarginsEqual(m_MarginBottom, d.m_MarginBottom)
        && MarginsEqual(m_MarginHeader, d.m_MarginHeader)
        && MarginsEqual(m_MarginFooter, d.m_MarginFooter)
        && m_PrintGridLines == d.m_PrintGridLines
        && m_PrintHeadings == d.m_PrintHeadings
        && m_HorizontalCentered == d.m_HorizontalCentered
        && m_VerticalCentered == d.m_VerticalCentered
        && m_AutoPageBreaks == d.m_AutoPageBreaks;
}

} // namespace SkSpreadSheet
