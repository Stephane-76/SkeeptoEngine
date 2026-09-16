//==============================================================================
// TestSkFormat
// Test la librairie Root Fichier
//==============================================================================

#include "../include/TestSkFormat.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

// We can send it to the API of a feature 
TestSkFormat::TestSkFormat() :CPPUNIT_NS::TestFixture(), m_Application(nullptr),m_FormatRoot(nullptr) {};

#ifdef _DEBUG
void TestSkFormat::Debug() {
	return;
	cout << "Debug Format ================================================================" << endl;
	cout << m_FormatRoot->DebugFormat();
	cout << "Debug Margin =========================================" << endl;
	cout << m_FormatRoot->DebugMargin();
	cout << "Debug Padding ========================================" << endl;
	cout << m_FormatRoot->DebugPadding();
	cout << "Debug BoxShadow ======================================" << endl;
	cout << m_FormatRoot->DebugShadow();
	cout << "Debug Font ===========================================" << endl;
	cout << m_FormatRoot->DebugFont();
	cout << "Debug Text ===========================================" << endl;
	cout << m_FormatRoot->DebugText();
	cout << "Debug BorderRect =====================================" << endl;
	cout << m_FormatRoot->DebugBorderRect();
}
#endif

void TestSkFormat::TestFormat() {
	tAllocatorRef wFormatRef;
	tFormatCss*   wFormatCssPool;
	

	// Make Color
	wFormatCssPool=m_FormatRoot->AllocFormat("C1");
	wFormatCssPool->Color().ColorHex("0xfffefffe");
	wFormatCssPool->BackgroundColor().Color(tColor(t_Color::AliceBlue));
    tUnitRectCss wMarginC1;
    wMarginC1.Left(tUnitCss(1, tUnitMetrics::pixels));
    m_FormatRoot->Margin(wMarginC1);

    wFormatRef=m_FormatRoot->Validate();
    CPPUNIT_ASSERT_MESSAGE("TestFormat C1", wFormatRef == 1);

	// Make Same Format
	wFormatCssPool= m_FormatRoot->AllocFormat("C2");
	wFormatCssPool->Color().ColorHex("0xfffefffe");
	wFormatCssPool->BackgroundColor().Color(tColor(t_Color::AliceBlue));
    
    tUnitRectCss wMarginC2;
    wMarginC2.Right(tUnitCss(7, tUnitMetrics::centimeters));
    m_FormatRoot->Margin(wMarginC2);

	wFormatRef = m_FormatRoot->Validate();
    CPPUNIT_ASSERT_MESSAGE("TestFormat C1", wFormatRef == 2);

	//cout << "Alloc ref: " << wFormatRef << endl;
#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	// Make Color Width Font
	wFormatCssPool = m_FormatRoot->AllocFormat("C3");
	wFormatCssPool->Color().ColorHex("0xfffeeefe");
	wFormatCssPool->BackgroundColor().Color(t_Color::AntiqueWhite);

	tFontCss wFont;
	wFont.Name("Arial");
	wFont.Family(tFontFamily::serif);
	wFont.Size(tUnitCss(8, tUnitMetrics::points));
	wFont.ObliqueDegrees(90);
	wFont.StretchPercent(50);
	wFont.LineHeight(tUnitCss(3, tUnitMetrics::number));
	//wFont.Style(tFontStyle::italic);
	m_FormatRoot->Font(wFont);
	
	wFormatRef = m_FormatRoot->Validate();
#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif


#ifdef _DEBUG
	Debug();
#endif

	wFormatCssPool = m_FormatRoot->AllocFormat("C4");
	wFormatCssPool->Color().ColorHex("0xfffeeefe");
	wFormatCssPool->BackgroundColor().Color(tColor(t_Color::AntiqueWhite));

	wFont.Name("Arial");
	wFont.Size(tUnitCss(2, tUnitMetrics::centimeters));
	m_FormatRoot->Font(wFont);

	m_FormatRoot->Validate();
#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	// Modify Second change color font add Text Margin padding Border
	m_FormatRoot->ModifyFormat("C4");

	wFormatCssPool->Color().ColorHex("0xfffefffe");

	wFont.Name("Helvetica");
	wFont.Size(tUnitCss(12, tUnitMetrics::inches));
	m_FormatRoot->Font(wFont);

	tTextCss wText;
	wText.TextAlign(tTextAlign::center);
	wText.VerticalTextAlign(tVerticalTextAlign::middle);
	wText.FormatString("#,##0.00");
	m_FormatRoot->Text(wText);
	m_FormatRoot->Validate();
#ifdef _DEBUG
    Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	wFormatCssPool = m_FormatRoot->AllocFormat("C5");
    tUnitRectCss wMargin;

	wMargin.Left(tUnitCss(1, tUnitMetrics::pixels));
	wMargin.Right(tUnitCss(1, tUnitMetrics::pixels));
	m_FormatRoot->Margin(wMargin);

	tUnitRectCss wPadding;
	wPadding.Top(tUnitCss(2, tUnitMetrics::pixels));
	wPadding.Bottom(tUnitCss(2, tUnitMetrics::pixels));
	m_FormatRoot->Padding(wPadding);

	tShadowCss wShadow;
	wShadow.Color().Color(t_Color::Black);
	m_FormatRoot->Shadow(wShadow);

	tBorderRectCss wBorderRect;
	wBorderRect.Left().BorderStyle(tBorderStyle::solid);
	wBorderRect.Left().Width(tUnitCss(1, tUnitMetrics::pixels));

	wBorderRect.TopLeftRadius().Unit(tUnitCss(50, tUnitMetrics::percent));

	m_FormatRoot->BorderRect(wBorderRect);

	wFont.Name("Arial");
	wFont.Size(tUnitCss(2, tUnitMetrics::centimeters));
	wFont.Variant(tFontVariant::small_caps);
	m_FormatRoot->Font(wFont);
	m_FormatRoot->Validate();

#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	// Modify Second change color font add Text Margin padding Border
	m_FormatRoot->ModifyFormat("C2");
	m_FormatRoot->Color().ColorHex("0xfffeeefe");
	tBorderRectCss wBorderRectC2;
	wBorderRectC2.Right().BorderStyle(tBorderStyle::solid);
	wBorderRectC2.Right().Width(tUnitCss(1, tUnitMetrics::pixels));
	m_FormatRoot->BorderRect(wBorderRectC2);

	m_FormatRoot->Validate();
    
#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	m_FormatRoot->AllocFormat("C21");
	tBorderRectCss wBorderRectC21;
	m_FormatRoot->BorderRect(wBorderRectC21);
	wBorderRectC21.Left().BorderStyle(tBorderStyle::solid);
	wBorderRectC21.Left().Width(tUnitCss(1, tUnitMetrics::pixels));
	wBorderRectC21.Left().Color().ColorHex("0xfffffffe");
	m_FormatRoot->BorderRect(wBorderRectC21);
	m_FormatRoot->Validate();

#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif
#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

	m_FormatRoot->ModifyFormat("C21");
	m_FormatRoot->Color().ColorHex("0xfffeeefe");
	tBorderRectCss wBorderRectC21_;
	m_FormatRoot->BorderRect(wBorderRectC21_);
	m_FormatRoot->Validate();

    tFormatCss* wFormatCss=m_FormatRoot->Format("C1");
    CPPUNIT_ASSERT_MESSAGE("TestFormat wFormatCss !=nullpre", wFormatCss != nullptr);

	m_FormatRoot->ModifyFormat("C1");
	m_FormatRoot->Color().ColorHex("0xfffeeefe");
	tBorderRectCss wBorderRectC1_;
	wBorderRectC1_.Left().BorderStyle(tBorderStyle::solid);
	wBorderRectC1_.Left().Width(tUnitCss(1, tUnitMetrics::pixels));
	m_FormatRoot->BorderRect(wBorderRectC1_);
	m_FormatRoot->Validate();


#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif
	
	// Same Border Rect
	m_FormatRoot->AllocFormat("C22");
	tBorderRectCss wBorderRectC22;
	m_FormatRoot->BorderRect(wBorderRectC22);
	wBorderRectC22.Left().BorderStyle(tBorderStyle::solid);
	wBorderRectC22.Left().Width(tUnitCss(1, tUnitMetrics::pixels));

	m_FormatRoot->BorderRect(wBorderRectC22);
	m_FormatRoot->Validate();


#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif
   
    m_FormatRoot->MergeInPlace("C1", "C2");
    
    tFormatCss* wFormatC1C2=m_FormatRoot->Format("@C1@C2");
    
    CPPUNIT_ASSERT_MESSAGE("TestFormat @C1@C2", wFormatC1C2 !=nullptr);


#ifdef _DEBUG
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif

#ifdef _DEBUG
	Debug();
#endif
	// Test JSON ==============================================================
	tString wJson = m_FormatRoot->WriteJson();
	//cout << wJson << endl;
#ifdef checkfo
    m_FormatRoot->Check();
#endif

	m_FormatRoot->ReadJson(wJson);
#ifdef _DEBUG
	Debug();
#endif
	m_FormatRoot->DeleteFormat("C1");
#ifdef _DEBUG
	Debug();
#endif
	m_FormatRoot->DeleteFormat("C2");
#ifdef _DEBUG
	Debug();
#endif
	m_FormatRoot->DeleteFormat("C3");
#ifdef _DEBUG
	Debug();
#endif
	m_FormatRoot->DeleteFormat("C4");
#ifdef _DEBUG
	Debug();
#endif
	m_FormatRoot->DeleteFormat("C5");
#ifdef _DEBUG
	Debug();
#endif

	m_FormatRoot->DeleteFormat("C21");
	
	m_FormatRoot->DeleteFormat("C22");
#ifdef _DEBUG
	Debug();
#endif
    
	tFormatCss* wFormatCssMerge = m_FormatRoot->Format("@C1@C2");
#ifdef checkfo
    m_FormatRoot->Check();
#endif

	if (wFormatCssMerge != nullptr) {
		//cout << wFormatCssMerge->CssStr(m_FormatRoot,true) << endl;
		m_FormatRoot->DeleteFormat("@C1@C2");
        
        
	}
#ifdef drawdebug
	Debug();
#endif
#ifdef checkfo
	m_FormatRoot->Check();
#endif
}

void TestSkFormat::setUp() {
	m_Application = tApplication::Instance();
	m_FormatRoot = tFormatRoot::Instance();

};

void TestSkFormat::tearDown() {
    TestSkFormatTeardown::AssertFormatRootEmptyAfterTeardown(m_FormatRoot, "TestSkFormat");
    DoneFormatRoot();
}
