//=============================================================================
// TestSkFunctionFinancial.cpp
//=============================================================================
#include "../include/TestSkFunctionFinancial.hpp"

TestSkFunctionFinancial::TestSkFunctionFinancial() : CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

TestSkFunctionFinancial::~TestSkFunctionFinancial() {

}

void TestSkFunctionFinancial::TestFunctionFinancial() {
    // Test PMT function - Payment
    // Example: Loan of 100,000 at 5% annual rate for 60 months
    // PMT(0.05/12, 60, 100000) should return approximately -1887.12
    m_Api->UndoCellValue("A1", "=PMT(0.05/12, 60, 100000)");
    tVariant wVariant = m_Api->CellValue("A1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1887.12, 0.01);
    
    // Test PMT with FV (future value)
    m_Api->UndoCellValue("A2", "=PMT(0.05/12, 60, 100000, 5000)");
    wVariant = m_Api->CellValue("A2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1960.65, 0.01);
    
    // Test PMT with type (beginning of period)
    m_Api->UndoCellValue("A3", "=PMT(0.05/12, 60, 100000, 0, 1)");
    wVariant = m_Api->CellValue("A3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1879.29, 0.01);
    
    // Test PMT with rate = 0
    m_Api->UndoCellValue("A4", "=PMT(0, 60, 100000)");
    wVariant = m_Api->CellValue("A4");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1666.67, 0.01); // -100000/60
    
    // Test IPMT function - Interest Payment
    // IPMT(0.05/12, 1, 60, 100000) - interest for first month
    m_Api->UndoCellValue("B1", "=IPMT(0.05/12, 1, 60, 100000)");
    wVariant = m_Api->CellValue("B1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -416.67, 0.01);
    
    // IPMT for last month (month 60)
    m_Api->UndoCellValue("B2", "=IPMT(0.05/12, 60, 60, 100000)");
    wVariant = m_Api->CellValue("B2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -7.83038740415425, 0.0001);
    
    // Test PPMT function - Principal Payment
    // PPMT(0.05/12, 1, 60, 100000) - principal for first month
    m_Api->UndoCellValue("C1", "=PPMT(0.05/12, 1, 60, 100000)");
    wVariant = m_Api->CellValue("C1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1470.45669773443, 0.0001);
    
    // PPMT for last month (month 60)
    // PPMT = PMT - wIpmt where wIpmt is positive (raw calculation)
    // PPMT = -1887.12 - 7.83038740415425 = -1894.95038740415425
    m_Api->UndoCellValue("C2", "=PPMT(0.05/12, 60, 60, 100000)");
    wVariant = m_Api->CellValue("C2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1879.29297699694, 0.0001);
    
    // Verify that IPMT + PPMT gives the expected sum
    // IPMT = -416.667, PPMT = -2303.79, so IPMT + PPMT = -2720.457
    m_Api->UndoCellValue("C3", "=IPMT(0.05/12, 1, 60, 100000) + PPMT(0.05/12, 1, 60, 100000)");
    wVariant = m_Api->CellValue("C3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -1887.1233644011, 0.01);
    
    // Test PV function - Present Value
    // PV(0.05/12, 60, -1887.12) should return approximately 100000
    m_Api->UndoCellValue("D1", "=PV(0.05/12, 60, -1887.12)");
    wVariant = m_Api->CellValue("D1");
    //cout << m_Api->Cell("D1")->FormulaStr() << endl;
    //cout << endl << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 100000.0, 1.0);
    
    // Test PV with FV
    m_Api->UndoCellValue("D2", "=PV(0.05/12, 60, -1887.12, 5000)");
    wVariant = m_Api->CellValue("D2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 96103.7947664245, 1.0);
    
    // Test PV with type
    m_Api->UndoCellValue("D3", "=PV(0.05/12, 60, -1880.18, 0, 1)");
    wVariant = m_Api->CellValue("D3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 100047.199825355, 1.0);
    
    // Test FV function - Future Value
    // FV(0.05/12, 60, -1887.12, 100000) should return approximately 0
    m_Api->UndoCellValue("E1", "=FV(0.05/12, 60, -1887.12, 100000)");
    wVariant = m_Api->CellValue("E1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.0, 1.0);
    
    // Test FV with savings (no initial PV)
    m_Api->UndoCellValue("E2", "=FV(0.05/12, 60, -100, 0)");
    wVariant = m_Api->CellValue("E2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 6800.61, 0.01);
    
    // Test FV with initial PV
    m_Api->UndoCellValue("E3", "=FV(0.05/12, 60, -100, -10000)");
    wVariant = m_Api->CellValue("E3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 19634.1950691194, 1.0);
    
    // Test RATE function - Interest Rate
    // RATE(60, -1887.12, 100000) should return approximately 0.05/12
    m_Api->UndoCellValue("F1", "=RATE(60, -1887.12, 100000)");
    wVariant = m_Api->CellValue("F1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.05/12, 0.0001);
    
    // Test RATE with FV
    m_Api->UndoCellValue("F2", "=RATE(60, -1887.12, 100000, 5000)");
    wVariant = m_Api->CellValue("F2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.00275247356124495, 0.0001);
    
    // Test NPER function - Number of Periods
    // NPER(0.05/12, -1887.12, 100000) should return approximately 60
    m_Api->UndoCellValue("G1", "=NPER(0.05/12, -1887.12, 100000)");
    wVariant = m_Api->CellValue("G1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 60.0, 0.1);
    
    // Test NPER with FV
    m_Api->UndoCellValue("G2", "=NPER(0.05/12, -1887.12, 100000, 5000)");
    wVariant = m_Api->CellValue("G2");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 62.6406289843026, 0.1);
    
    // Test NPER with rate = 0
    m_Api->UndoCellValue("G3", "=NPER(0, -1666.67, 100000)");
    wVariant = m_Api->CellValue("G3");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 60.0, 0.1);
    
    // Test error cases
    // PMT with invalid arguments
    m_Api->UndoCellValue("H1", "=PMT(0.05/12, -10, 100000)");
    wVariant = m_Api->CellValue("H1");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // IPMT with per > nper
    m_Api->UndoCellValue("H2", "=IPMT(0.05/12, 70, 60, 100000)");
    wVariant = m_Api->CellValue("H2");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // IPMT with per < 1
    m_Api->UndoCellValue("H3", "=IPMT(0.05/12, 0, 60, 100000)");
    wVariant = m_Api->CellValue("H3");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // Test NPV function - Net Present Value
    // Example: NPV(0.1, -10000, 3000, 4200, 6800) should return approximately 1188.44
    m_Api->UndoCellValue("I1", -10000);
    m_Api->UndoCellValue("I2", 3000);
    m_Api->UndoCellValue("I3", 4200);
    m_Api->UndoCellValue("I4", 6800);
    m_Api->UndoCellValue("I5", "=NPV(0.1, I1:I4)");
    wVariant = m_Api->CellValue("I5");
    //cout << endl << "I5: =NPV(0.1, I1:I4)=" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 1188.44, 0.01);

    // Same cash flows as separate args (PopArgs LIFO must not reverse period order).
    m_Api->UndoCellValue("I6", "=NPV(0.1, -10000, 3000, 4200, 6800)");
    wVariant = m_Api->CellValue("I6");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 1188.44, 0.01);
    
    // Test IRR function - Internal Rate of Return
    // Example: IRR with cash flows -70000, 12000, 15000, 18000, 21000, 26000
    m_Api->UndoCellValue("J1", -70000);
    m_Api->UndoCellValue("J2", 12000);
    m_Api->UndoCellValue("J3", 15000);
    m_Api->UndoCellValue("J4", 18000);
    m_Api->UndoCellValue("J5", 21000);
    m_Api->UndoCellValue("J6", 26000);
    m_Api->UndoCellValue("J7", "=IRR(J1:J6)");
    wVariant = m_Api->CellValue("J7");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.0866, 0.001);
    
    // Test MIRR function - Modified Internal Rate of Return
    // Example: MIRR with cash flows -120000, 39000, 30000, 21000, 37000, 46000
    m_Api->UndoCellValue("K1", -120000);
    m_Api->UndoCellValue("K2", 39000);
    m_Api->UndoCellValue("K3", 30000);
    m_Api->UndoCellValue("K4", 21000);
    m_Api->UndoCellValue("K5", 37000);
    m_Api->UndoCellValue("K6", 46000);
    m_Api->UndoCellValue("K7", "=MIRR(K1:K6, 0.1, 0.12)");
    wVariant = m_Api->CellValue("K7");
    //cout << endl << "K7: =MIRR(K1:K6, 0.1, 0.12)" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.1261, 0.001);
    
    // Test DB function - Declining Balance depreciation
    // Example: DB(1000000, 100000, 6, 1, 7) - first year depreciation
    m_Api->UndoCellValue("L1", "=DB(1000000, 100000, 6, 1, 7)");
    wVariant = m_Api->CellValue("L1");
    //cout << endl << "L1: =DB(1000000, 100000, 6, 1, 7)=" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 186083.33, 0.01);
    
    // Test DDB function - Double Declining Balance depreciation
    // Example: DDB(2400, 300, 10, 1) - first year depreciation
    m_Api->UndoCellValue("M1", "=DDB(2400, 300, 10, 1)");
    wVariant = m_Api->CellValue("M1");
    //cout << endl << "M1: =DDB(2400, 300, 10, 1)=" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 480.0, 0.01);
    
    // Test SLN function - Straight Line depreciation
    // Example: SLN(30000, 7500, 10) - annual depreciation
    m_Api->UndoCellValue("N1", "=SLN(30000, 7500, 10)");
    wVariant = m_Api->CellValue("N1");
    //cout << endl << "N1: =SLN(30000, 7500, 10)=" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 2250.0, 0.01);
    
    // Test SYD function - Sum of Years Digits depreciation
    // Example: SYD(30000, 7500, 10, 1) - first year depreciation
    m_Api->UndoCellValue("O1", "=SYD(30000, 7500, 10, 1)");
    wVariant = m_Api->CellValue("O1");
    //cout << endl << "D1: =SYD(30000, 7500, 10, 1)=" << wVariant << endl;
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 4090.91, 0.01);
    
    // Test PMT first to verify it matches Excel
    m_Api->UndoCellValue("P0", "=PMT(0.1/12, 36, 100000)");
    wVariant = m_Api->CellValue("P0");
    //cout << endl << "P0: =PMT(0.1/12, 36, 100000)=" << wVariant << endl;
    
    // Test CUMIPMT function - Cumulative Interest Payment
    // Example: CUMIPMT(0.1/12, 36, 100000, 1, 12, 0) - interest for first year
    m_Api->UndoCellValue("P1", "=CUMIPMT(0.1/12, 36, 100000, 1, 12, 0)");
    // CUMIPMT(0,1/12; 36; 100000; 1; 12; 0)
    wVariant = m_Api->CellValue("P1");
    //cout << endl << "P:=CUMIPMT(0.1/12, 36, 100000, 1, 12, 0)=" << wVariant << endl;
    // Note: Calculated value is -7827.85, test expects -8646.37 - need to verify correct Excel value
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -8646.37, 1.0);
    
    // Test CUMPRINC function - Cumulative Principal Payment
    // Example: CUMPRINC(0.1/12, 36, 100000, 1, 12, 0) - principal for first year
    // Excel calculates CUMPRINC as: Balance before start_period - Balance after end_period
    // Excel returns: -30074.25 (matches our implementation)
    m_Api->UndoCellValue("Q1", "=CUMPRINC(0.1/12, 36, 100000, 1, 12, 0)");
    wVariant = m_Api->CellValue("Q1");
    //cout << endl << "Q1: =CUMPRINC(0.1/12, 36, 100000, 1, 12, 0)=" << wVariant << endl;
    wVariant = m_Api->CellValue("Q1");
    //cout << "Q1=CUMPRINC(0.1/12, 36, 100000, 1, 12, 0)=" << wVariant << endl;
    // Excel returns -30074.25, which matches our balance difference method
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), -30074.2, 1.0);
    
    // Test EFFECT function - Effective Annual Interest Rate
    // Example: EFFECT(0.1, 4) - effective rate for 10% nominal with quarterly compounding
    m_Api->UndoCellValue("R1", "=EFFECT(0.1, 4)");
    wVariant = m_Api->CellValue("R1");
    //cout << "Q1=EFFECT(0.1, 4)=" << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.1038, 0.0001);
    
    // Test NOMINAL function - Nominal Annual Interest Rate
    // Example: NOMINAL(0.1038, 4) - nominal rate for 10.38% effective with quarterly compounding
    m_Api->UndoCellValue("S1", "=NOMINAL(0.1038, 4)");
    wVariant = m_Api->CellValue("S1");
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.1, 0.0001);
    
    // Test PDURATION function - Number of Periods to Reach Value
    // Example: PDURATION(0.1, 1000, 2000) - periods to double investment at 10% rate
    m_Api->UndoCellValue("T1", "=PDURATION(0.1, 1000, 2000)");
    wVariant = m_Api->CellValue("T1");
    // Expected: log(2000/1000) / log(1.1) = log(2) / log(1.1) ≈ 7.27
    //cout << m_Api->Cell("T1")->FormulaStr() << endl;
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 7.27, 0.1);
    
    // Test RRI function - Equivalent Interest Rate
    // Example: RRI(10, 1000, 2000) - equivalent rate to double in 10 periods
    m_Api->UndoCellValue("T2", "=RRI(10, 1000, 2000)");
    wVariant = m_Api->CellValue("T2");
    // Expected: (2000/1000)^(1/10) - 1 = 2^(0.1) - 1 ≈ 0.07177 (7.18%)
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 0.07177, 0.001);
    
    // Test FVSCHEDULE function - Future Value with Variable Interest Rates
    // Example: FVSCHEDULE(1000, {0.1, 0.12, 0.15}) - FV with rates 10%, 12%, 15%
    m_Api->UndoCellValue("U1", 1000);
    m_Api->UndoCellValue("U2", 0.1);
    m_Api->UndoCellValue("U3", 0.12);
    m_Api->UndoCellValue("U4", 0.15);
    m_Api->UndoCellValue("T3", "=FVSCHEDULE(U1, U2:U4)");
    wVariant = m_Api->CellValue("T3");
    // Expected: 1000 * 1.1 * 1.12 * 1.15 = 1416.8
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 1416.8, 1.0);
    
    // Test VDB function - Variable Declining Balance Depreciation
    // Example: VDB(100000, 10000, 10, 0, 1, 2) - depreciation for first period
    m_Api->UndoCellValue("T4", "=VDB(100000, 10000, 10, 0, 1, 2)");
    wVariant = m_Api->CellValue("T4");
    // Expected: First period depreciation with factor 2 (double declining balance)
    // Should be approximately 20000 (20% of 100000)
    //cout << m_Api->Cell("T4")->FormulaStr() << endl;
    //cout << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 20000.0, 100.0);
    
    // Test XNPV function - Net Present Value for Non-Periodic Cash Flows
    // Example: XNPV(0.1, {-1000, 500, 600}, {DATE(2020,1,1), DATE(2020,6,1), DATE(2021,1,1)})
    // Note: We'll use numeric dates (Excel serial dates)
    // DATE(2020,1,1) = 43831, DATE(2020,6,1) = 43983, DATE(2021,1,1) = 44197
    // Days from first date: 0, 152, 366
    // Years: 0, 0.4164, 1.0027
    // XNPV = -1000/(1.1)^0 + 500/(1.1)^0.4164 + 600/(1.1)^1.0027 ≈ 26.1
    m_Api->UndoCellValue("V1", -1000);
    m_Api->UndoCellValue("V2", 500);
    m_Api->UndoCellValue("V3", 600);
    m_Api->UndoCellValue("W1", 43831);
    m_Api->UndoCellValue("W2", 43983);
    m_Api->UndoCellValue("W3", 44197);
    m_Api->UndoCellValue("T5", "=XNPV(0.1, V1:V3, W1:W3)");
    wVariant = m_Api->CellValue("T5");
    // Expected: XNPV ≈ 26.1 (positive, investment is profitable)
    //cout << m_Api->Cell("T5")->FormulaStr() << endl;
    //cout << endl << wVariant << endl;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), 26.1, 5.0);
    
    // Test XIRR function - Internal Rate of Return for Non-Periodic Cash Flows
    // Example: XIRR({-1000, 500, 600}, {DATE(2020,1,1), DATE(2020,6,1), DATE(2021,1,1)}, 0.1)
    // XIRR solves: -1000 + 500/(1+r)^0.4164 + 600/(1+r)^1.0027 = 0
    // Expected rate should be around 0.15-0.25 (15-25%)
    m_Api->UndoCellValue("T6", "=XIRR(V1:V3, W1:W3, 0.1)");
    wVariant = m_Api->CellValue("T6");
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT(wVariant.IsDouble() || wVariant.IsInt());
    CPPUNIT_ASSERT(wVariant.Double() > 0.1 && wVariant.Double() < 0.3);

    // 2-arg form (default guess) — same LIFO reverse path as ADDRESS/XLOOKUP.
    // Use a valid A1 ref (T6B is not a legal column+row address).
    m_Api->UndoCellValue("T16", "=XIRR(V1:V3, W1:W3)");
    wVariant = m_Api->CellValue("T16");
    CPPUNIT_ASSERT(!wVariant.IsError());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(wVariant.Double(), m_Api->CellValue("T6").Double(), 1e-6);
    
    // Test error cases for new functions
    // PDURATION with invalid arguments
    m_Api->UndoCellValue("T7", "=PDURATION(0, 1000, 2000)");
    wVariant = m_Api->CellValue("T7");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // RRI with invalid arguments
    m_Api->UndoCellValue("T8", "=RRI(0, 1000, 2000)");
    wVariant = m_Api->CellValue("T8");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // VDB with invalid period
    m_Api->UndoCellValue("T9", "=VDB(100000, 10000, 10, 5, 3, 2)");
    wVariant = m_Api->CellValue("T9");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // XNPV with mismatched arrays
    m_Api->UndoCellValue("X1", "100");
    m_Api->UndoCellValue("X2", "200");
    m_Api->UndoCellValue("T10", "=XNPV(0.1, V1:V3, X1:X2)");
    wVariant = m_Api->CellValue("T10");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // XIRR with mismatched arrays
    m_Api->UndoCellValue("T11", "=XIRR(V1:V3, X1:X2, 0.1)");
    wVariant = m_Api->CellValue("T11");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // NPV with invalid rate
    m_Api->UndoCellValue("T12", "=NPV(-0.1, I1:I4)");
    wVariant = m_Api->CellValue("T12");
    // NPV should handle negative rates (may return error or calculate)
    
    // IRR with invalid cash flows
    m_Api->UndoCellValue("T2", "=IRR(J1:J1)");
    wVariant = m_Api->CellValue("T2");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // DB with invalid period
    m_Api->UndoCellValue("T3", "=DB(1000000, 100000, 6, 0)");
    wVariant = m_Api->CellValue("T3");
    CPPUNIT_ASSERT(wVariant.IsError());
    
    // SLN with invalid life
    m_Api->UndoCellValue("T4", "=SLN(30000, 7500, 0)");
    wVariant = m_Api->CellValue("T4");
    CPPUNIT_ASSERT(wVariant.IsError());
}

void TestSkFunctionFinancial::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();

    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_Application->Locale()->Lang("us");
}

void TestSkFunctionFinancial::tearDown() {
    m_Application->Locale()->Lang("fr");
    delete(m_Api);
}
