//=============================================================================
// SkExcelFormulaMarkersSelfTest.cpp
// Regression coverage for TransFormFormulaSyntax OOXML-marker handling.
//=============================================================================
#include <SkExcelFormulaMarkersSelfTest.hpp>
#include <SkExcelTools.hpp>
#include <SkTypes.hpp>
#include <iostream>

using namespace SkRoot;

namespace SkExcel {

namespace {

// Run one case through the basic (file-independent) transform and compare.
tBool CheckCase(const tString& sLabel, tString sInput, const tString& sExpected, tInt& ioFailures) {
    tString wActual = sInput;
    TransFormFormulaSyntax(wActual);
    const tBool wOk = (wActual == sExpected);
    if (!wOk) {
        ++ioFailures;
        std::cout << "  FAIL [" << sLabel << "]\n"
                  << "      input   : " << sInput << "\n"
                  << "      expected: " << sExpected << "\n"
                  << "      actual  : " << wActual << std::endl;
    } else {
        std::cout << "  ok   [" << sLabel << "] -> " << wActual << std::endl;
    }
    return wOk;
}

} // namespace

tInt RunFormulaMarkersSelfTest() {
    std::cout << "RunFormulaMarkersSelfTest()" << std::endl;
    tInt wFailures = 0;

    // Future-function prefix.
    CheckCase("xlfn SEQUENCE", "_xlfn.SEQUENCE(COUNTA(D17:D36))",
              "SEQUENCE(COUNTA(D17:D36))", wFailures);

    // Excel UI D14# is stored as _xlfn.ANCHORARRAY(D14) (League-Table-Examples Part B).
    CheckCase("ANCHORARRAY cell", "_xlfn.ANCHORARRAY(D14)", "D14#", wFailures);
    CheckCase("SEQUENCE ANCHORARRAY", "_xlfn.SEQUENCE(COUNTA(_xlfn.ANCHORARRAY(D14)))",
              "SEQUENCE(COUNTA(D14#))", wFailures);
    CheckCase("ANCHORARRAY spaced", "COUNTA(_xlfn.ANCHORARRAY( D14))",
              "COUNTA(D14#)", wFailures);
    CheckCase("ANCHORARRAY sheet", "ANCHORARRAY('Part B'!D14)", "'Part B'!D14#", wFailures);
    CheckCase("ANCHORARRAY inside string", "IF(A1=\"ANCHORARRAY(D14)\",1,0)",
              "IF(A1=\"ANCHORARRAY(D14)\",1,0)", wFailures);

    // Worksheet-only dynamic-array prefix.
    CheckCase("xlfn._xlws SORT", "_xlfn._xlws.SORT(A1:A3)", "SORT(A1:A3)", wFailures);

    // LET with parameter-name prefix on both the binding and the body.
    CheckCase("LET params", "_xlfn.LET(_xlpm.x,5,_xlpm.x*2)", "LET(x,5,x*2)", wFailures);

    // Structured reference must be left intact after the UNIQUE prefix is removed.
    CheckCase("UNIQUE structured", "_xlfn.UNIQUE(DataTable[team_home])",
              "UNIQUE(DataTable[team_home])", wFailures);

    // Literal string array constant: braces must be preserved (this is the C16 regression).
    CheckCase("string array constant", "{\"POS\",\"TEAM\",\"P\"}",
              "{\"POS\",\"TEAM\",\"P\"}", wFailures);

    // Numeric array constant preserved too.
    CheckCase("numeric array constant", "{9,8,6,1}", "{9,8,6,1}", wFailures);

    // A marker-looking substring inside a string literal must NOT be stripped.
    CheckCase("marker inside string", "IF(A1=\"_xlpm.x\",1,0)",
              "IF(A1=\"_xlpm.x\",1,0)", wFailures);

    // Dotted Excel stats names -> undotted sker names (lexer cannot keep '.' in IDs).
    CheckCase("STDEV.S rewrite", "STDEV.S(A1:A3)", "STDEV_S(A1:A3)", wFailures);
    CheckCase("STDEV.P rewrite", "STDEV.P(A1:A3)", "STDEV_P(A1:A3)", wFailures);
    CheckCase("VAR.S rewrite", "VAR.S(A1:A3)", "VAR_S(A1:A3)", wFailures);
    CheckCase("VAR.P rewrite", "VAR.P(A1:A3)", "VAR_P(A1:A3)", wFailures);
    CheckCase("COVARIANCE.P rewrite", "COVARIANCE.P(A1:A3,B1:B3)", "COVARIANCE_P(A1:A3,B1:B3)", wFailures);
    CheckCase("PERCENTILE.INC rewrite", "PERCENTILE.INC(A1:A3,0.5)", "PERCENTILE_INC(A1:A3,0.5)", wFailures);
    CheckCase("FORECAST.LINEAR rewrite", "FORECAST.LINEAR(1,A1:A3,B1:B3)",
              "FORECAST_LINEAR(1,A1:A3,B1:B3)", wFailures);
    CheckCase("PERCENTRANK.INC rewrite", "PERCENTRANK.INC(A1:A3,2)",
              "PERCENTRANK_INC(A1:A3,2)", wFailures);
    CheckCase("PERCENTRANK.EXC rewrite", "PERCENTRANK.EXC(A1:A3,2)",
              "PERCENTRANK_EXC(A1:A3,2)", wFailures);
    CheckCase("SKEW.P rewrite", "SKEW.P(A1:A5)", "SKEW_P(A1:A5)", wFailures);
    CheckCase("RANK.AVG rewrite", "RANK.AVG(A1,A1:A10,0)", "RANK_AVG(A1,A1:A10,0)", wFailures);
    CheckCase("GAMMALN.PRECISE rewrite", "GAMMALN.PRECISE(A1)", "GAMMALN_PRECISE(A1)", wFailures);
    CheckCase("NORM.S.DIST rewrite", "NORM.S.DIST(A1,TRUE)", "NORM_S_DIST(A1,TRUE)", wFailures);
    CheckCase("NORM.S.INV rewrite", "NORM.S.INV(0.9)", "NORM_S_INV(0.9)", wFailures);
    CheckCase("NORM.DIST rewrite", "NORM.DIST(A1,0,1,TRUE)", "NORM_DIST(A1,0,1,TRUE)", wFailures);
    CheckCase("NORM.INV rewrite", "NORM.INV(0.9,40,1.5)", "NORM_INV(0.9,40,1.5)", wFailures);
    CheckCase("MODE.SNGL rewrite", "MODE.SNGL(A1:A3)", "MODE_SNGL(A1:A3)", wFailures);
    CheckCase("MODE.MULT rewrite", "MODE.MULT(A1:A3)", "MODE_MULT(A1:A3)", wFailures);
    CheckCase("RANK.EQ rewrite", "RANK.EQ(A1,A1:A10,0)", "RANK_EQ(A1,A1:A10,0)", wFailures);
    CheckCase("CEILING.MATH rewrite", "CEILING.MATH(A1,1)", "CEILING_MATH(A1,1)", wFailures);
    CheckCase("FLOOR.MATH rewrite", "FLOOR.MATH(-2.5,1,-1)", "FLOOR_MATH(-2.5,1,-1)", wFailures);
    CheckCase("CEILING.PRECISE rewrite", "CEILING.PRECISE(A1,1)", "CEILING_PRECISE(A1,1)", wFailures);
    CheckCase("FLOOR.PRECISE rewrite", "FLOOR.PRECISE(-2.5,1)", "FLOOR_PRECISE(-2.5,1)", wFailures);
    CheckCase("ISO.CEILING rewrite", "ISO.CEILING(A1,1)", "ISO_CEILING(A1,1)", wFailures);
    CheckCase("NETWORKDAYS.INTL rewrite", "NETWORKDAYS.INTL(A1,B1,1)",
              "NETWORKDAYS_INTL(A1,B1,1)", wFailures);
    CheckCase("WORKDAY.INTL rewrite", "WORKDAY.INTL(A1,1,11)",
              "WORKDAY_INTL(A1,1,11)", wFailures);
    CheckCase("ERROR.TYPE rewrite", "ERROR.TYPE(A1)",
              "ERROR_TYPE(A1)", wFailures);
    CheckCase("xlfn STDEV.S", "_xlfn.STDEV.S(B1:B10)", "STDEV_S(B1:B10)", wFailures);
    CheckCase("STDEV.S inside string kept", "IF(A1=\"STDEV.S\",1,0)",
              "IF(A1=\"STDEV.S\",1,0)", wFailures);

    // Whole-column INDEX/MATCH must stay A:A / $B:$B (not collapse to $A$1:$A$1).
    {
        const tString wIn =
            "INDEX('CA 2025'!A:A,MATCH($B134,'CA 2025'!$B:$B,0))";
        const tString wOut = ConvertA1CellRefsToR1C1(wIn, 134, 1);
        if (wOut.find("A:A") == tString::npos || wOut.find("$B:$B") == tString::npos) {
            ++wFailures;
            std::cout << "  FAIL [ConvertA1 whole-column INDEX/MATCH]\n"
                      << "      input   : " << wIn << "\n"
                      << "      actual  : " << wOut << std::endl;
        } else {
            std::cout << "  ok   [ConvertA1 whole-column INDEX/MATCH] -> " << wOut
                      << std::endl;
        }
    }

    if (wFailures == 0) {
        std::cout << "RunFormulaMarkersSelfTest: PASS" << std::endl;
        return 0;
    }
    std::cout << "RunFormulaMarkersSelfTest: FAIL (" << wFailures << ")" << std::endl;
    return wFailures;
}

} // namespace SkExcel
