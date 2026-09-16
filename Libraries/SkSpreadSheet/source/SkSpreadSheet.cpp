//=============================================================================
// SkSpreadSheet Container of WorkBook
//=============================================================================
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkLemonSpreadSheet.hpp"
#include "../include/SkSharedFormula.hpp"
#include "../include/SkUndoRedoSp.hpp"

#include <ctime>

#define _debugjson
#define _debugworkbook

namespace SkSpreadSheet {

	// One class For all application =======================================
	tSpreadSheetContainer* wStaticSpreadSheet = nullptr;
    // Init OnCellChange
    tOnCellChange* tSpreadSheetContainer::m_OnCellChange = nullptr;

    tSpreadSheetContainer::tSpreadSheetContainer() : tClass(), m_ActiveWorkBook(nullptr),m_FormatApi(nullptr), m_JsonRestoreCachedFormulaValues(false), m_CurrentJsonCell(nullptr) {
        // Set Shared
        m_JsonSharedString.Key(kJsonKeySharedString);
        m_JsonSharedFormula.Key(kJsonKeySharedFormula);
    
        // Initialize function
        // MATH ==============================================================
        m_FunctionDictionary.AddFunctionRef("SUM","Sum","Math", -1, new tFunctionSum);
        m_FunctionDictionary.AddFunctionRef("MIN","Min","Math", -1, new tFunctionMin);
        m_FunctionDictionary.AddFunctionRef("MAX","Max","Math", -1, new tFunctionMax);
        m_FunctionDictionary.AddFunctionRef("MAXA","MaxA","Math", -1, new tFunctionMinMaxA(true));
        m_FunctionDictionary.AddFunctionRef("MINA","MinA","Math", -1, new tFunctionMinMaxA(false));
        m_FunctionDictionary.AddFunctionRef("ROUND","Round","Math", 2, new tFunctionRound);
        m_FunctionDictionary.AddFunctionRef("ROUNDDOWN","RoundDown","Math", 2, new tFunctionRoundDown);
        m_FunctionDictionary.AddFunctionRef("ROUNDUP","RoundUp","Math", 2, new tFunctionRoundUp);
        m_FunctionDictionary.AddFunctionRef("TRUNC","Trunc","Math", 2, new tFunctionTrunc);
        m_FunctionDictionary.AddFunctionRef("CEILING_MATH","CeilingMath","Math", -1,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::CeilingMath));
        m_FunctionDictionary.AddFunctionRef("FLOOR_MATH","FloorMath","Math", -1,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::FloorMath));
        // CEILING.PRECISE / FLOOR.PRECISE / ISO.CEILING → undotted on import.
        m_FunctionDictionary.AddFunctionRef("CEILING_PRECISE","CeilingPrecise","Math", -1,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::CeilingPrecise));
        m_FunctionDictionary.AddFunctionRef("FLOOR_PRECISE","FloorPrecise","Math", -1,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::FloorPrecise));
        m_FunctionDictionary.AddFunctionRef("ISO_CEILING","IsoCeiling","Math", -1,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::CeilingPrecise));
        // Legacy CEILING / FLOOR (compatibility): significance required, same-sign rule.
        m_FunctionDictionary.AddFunctionRef("CEILING","Ceiling","Math", 2,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::CeilingLegacy));
        m_FunctionDictionary.AddFunctionRef("FLOOR","Floor","Math", 2,
            new tFunctionCeilingFloorMath(tCeilingFloorKind::FloorLegacy));
        m_FunctionDictionary.AddFunctionRef("MROUND","MRound","Math", 2, new tFunctionMRound);
        // Shared: false = GCD, true = LCM.
        m_FunctionDictionary.AddFunctionRef("GCD","Gcd","Math", -1, new tFunctionGcdLcm(false));
        m_FunctionDictionary.AddFunctionRef("LCM","Lcm","Math", -1, new tFunctionGcdLcm(true));
        m_FunctionDictionary.AddFunctionRef("INT","Int","Math", 1, new tFunctionInt);
        m_FunctionDictionary.AddFunctionRef("ABS","Abs","Abs", -1, new tFunctionAbs);
        m_FunctionDictionary.AddFunctionRef("AVERAGE","Average","Math", -1, new tFunctionAverage);
        m_FunctionDictionary.AddFunctionRef("AVERAGEA","AverageA","Math", -1, new tFunctionAverageA);
        m_FunctionDictionary.AddFunctionRef("MEDIAN","Median","Math", -1, new tFunctionMedian);
        // Shared variance family (STDEV_S/P, VAR_S/P, *A). Dotted Excel names rewritten on import.
        m_FunctionDictionary.AddFunctionRef("STDEV_S","Stdev_S","Math", -1, new tFunctionVariance(tVarianceKind::StdevS));
        m_FunctionDictionary.AddFunctionRef("STDEV","Stdev","Math", -1, new tFunctionVariance(tVarianceKind::StdevS));
        m_FunctionDictionary.AddFunctionRef("STDEV_P","Stdev_P","Math", -1, new tFunctionVariance(tVarianceKind::StdevP));
        m_FunctionDictionary.AddFunctionRef("VAR_S","Var_S","Math", -1, new tFunctionVariance(tVarianceKind::VarS));
        m_FunctionDictionary.AddFunctionRef("VAR","Var","Math", -1, new tFunctionVariance(tVarianceKind::VarS));
        m_FunctionDictionary.AddFunctionRef("VAR_P","Var_P","Math", -1, new tFunctionVariance(tVarianceKind::VarP));
        m_FunctionDictionary.AddFunctionRef("STDEVA","StdevA","Math", -1, new tFunctionVariance(tVarianceKind::StdevA));
        m_FunctionDictionary.AddFunctionRef("STDEVPA","StdevPA","Math", -1, new tFunctionVariance(tVarianceKind::StdevPA));
        m_FunctionDictionary.AddFunctionRef("VARA","VarA","Math", -1, new tFunctionVariance(tVarianceKind::VarA));
        m_FunctionDictionary.AddFunctionRef("VARPA","VarPA","Math", -1, new tFunctionVariance(tVarianceKind::VarPA));
        m_FunctionDictionary.AddFunctionRef("AVEDEV","AveDev","Math", -1, new tFunctionAveDev);
        m_FunctionDictionary.AddFunctionRef("DEVSQ","DevSq","Math", -1, new tFunctionDevSq);
        // SKEW.P / GAMMALN.PRECISE / RANK.AVG rewritten on import.
        m_FunctionDictionary.AddFunctionRef("SKEW","Skew","Math", -1, new tFunctionSkew(false));
        m_FunctionDictionary.AddFunctionRef("SKEW_P","Skew_P","Math", -1, new tFunctionSkew(true));
        m_FunctionDictionary.AddFunctionRef("KURT","Kurt","Math", -1, new tFunctionKurt);
        m_FunctionDictionary.AddFunctionRef("STANDARDIZE","Standardize","Math", 3, new tFunctionStandardize);
        m_FunctionDictionary.AddFunctionRef("FISHER","Fisher","Math", 1, new tFunctionFisher(false));
        m_FunctionDictionary.AddFunctionRef("FISHERINV","FisherInv","Math", 1, new tFunctionFisher(true));
        m_FunctionDictionary.AddFunctionRef("PHI","Phi","Math", 1, new tFunctionPhi);
        m_FunctionDictionary.AddFunctionRef("GAUSS","Gauss","Math", 1, new tFunctionGauss);
        // NORM.S.DIST / NORM.S.INV / NORM.DIST / NORM.INV rewritten on import.
        m_FunctionDictionary.AddFunctionRef("NORM_S_DIST","Norm_S_Dist","Math", 2, new tFunctionNormSDist(false));
        m_FunctionDictionary.AddFunctionRef("NORMSDIST","NormsDist","Math", 1, new tFunctionNormSDist(true));
        m_FunctionDictionary.AddFunctionRef("NORM_DIST","Norm_Dist","Math", 4, new tFunctionNormDist);
        m_FunctionDictionary.AddFunctionRef("NORMDIST","NormDist","Math", 4, new tFunctionNormDist);
        m_FunctionDictionary.AddFunctionRef("NORM_S_INV","Norm_S_Inv","Math", 1, new tFunctionNormInv(true));
        m_FunctionDictionary.AddFunctionRef("NORMSINV","NormsInv","Math", 1, new tFunctionNormInv(true));
        m_FunctionDictionary.AddFunctionRef("NORM_INV","Norm_Inv","Math", 3, new tFunctionNormInv(false));
        m_FunctionDictionary.AddFunctionRef("NORMINV","NormInv","Math", 3, new tFunctionNormInv(false));
        m_FunctionDictionary.AddFunctionRef("GAMMA","Gamma","Math", 1, new tFunctionGamma(false));
        m_FunctionDictionary.AddFunctionRef("GAMMALN","GammaLn","Math", 1, new tFunctionGamma(true));
        m_FunctionDictionary.AddFunctionRef("GAMMALN_PRECISE","GammaLn_Precise","Math", 1, new tFunctionGamma(true));
        m_FunctionDictionary.AddFunctionRef("CORREL","Correl","Math", 2, new tFunctionCorrel);
        m_FunctionDictionary.AddFunctionRef("PEARSON","Pearson","Math", 2, new tFunctionCorrel);
        // COVARIANCE.P / COVARIANCE.S rewritten on import.
        m_FunctionDictionary.AddFunctionRef("COVARIANCE_P","Covariance_P","Math", 2, new tFunctionCovariance(false));
        m_FunctionDictionary.AddFunctionRef("COVARIANCE_S","Covariance_S","Math", 2, new tFunctionCovariance(true));
        // PERCENTILE.INC / .EXC / QUARTILE.INC / .EXC rewritten; bare names = inclusive.
        m_FunctionDictionary.AddFunctionRef("PERCENTILE_INC","Percentile_Inc","Math", 2, new tFunctionPercentile(false));
        m_FunctionDictionary.AddFunctionRef("PERCENTILE_EXC","Percentile_Exc","Math", 2, new tFunctionPercentile(true));
        m_FunctionDictionary.AddFunctionRef("PERCENTILE","Percentile","Math", 2, new tFunctionPercentile(false));
        m_FunctionDictionary.AddFunctionRef("QUARTILE_INC","Quartile_Inc","Math", 2, new tFunctionQuartile(false));
        m_FunctionDictionary.AddFunctionRef("QUARTILE_EXC","Quartile_Exc","Math", 2, new tFunctionQuartile(true));
        m_FunctionDictionary.AddFunctionRef("QUARTILE","Quartile","Math", 2, new tFunctionQuartile(false));
        m_FunctionDictionary.AddFunctionRef("SLOPE","Slope","Math", 2, new tFunctionLinReg(tLinRegKind::Slope));
        m_FunctionDictionary.AddFunctionRef("INTERCEPT","Intercept","Math", 2, new tFunctionLinReg(tLinRegKind::Intercept));
        m_FunctionDictionary.AddFunctionRef("RSQ","Rsq","Math", 2, new tFunctionLinReg(tLinRegKind::Rsq));
        m_FunctionDictionary.AddFunctionRef("STEYX","Steyx","Math", 2, new tFunctionLinReg(tLinRegKind::Steyx));
        // FORECAST.LINEAR rewritten; FORECAST kept as compatibility alias.
        m_FunctionDictionary.AddFunctionRef("FORECAST_LINEAR","Forecast_Linear","Math", 3, new tFunctionForecast);
        m_FunctionDictionary.AddFunctionRef("FORECAST","Forecast","Math", 3, new tFunctionForecast);
        // TREND / GROWTH / LINEST — linear or exponential fit (single independent variable); spill.
        m_FunctionDictionary.AddFunctionRef("TREND","Trend","Math", -1, new tFunctionTrend);
        m_FunctionDictionary.AddFunctionRef("GROWTH","Growth","Math", -1, new tFunctionGrowth);
        m_FunctionDictionary.AddFunctionRef("LINEST","LinEst","Math", -1, new tFunctionLinEst);
        // PERCENTRANK.INC / .EXC rewritten; bare PERCENTRANK = inclusive (compatibility).
        m_FunctionDictionary.AddFunctionRef("PERCENTRANK_INC","PercentRank_Inc","Math", -1, new tFunctionPercentRank(false));
        m_FunctionDictionary.AddFunctionRef("PERCENTRANK_EXC","PercentRank_Exc","Math", -1, new tFunctionPercentRank(true));
        m_FunctionDictionary.AddFunctionRef("PERCENTRANK","PercentRank","Math", -1, new tFunctionPercentRank(false));
        m_FunctionDictionary.AddFunctionRef("PERMUT","Permut","Math", 2, new tFunctionPermut(false));
        m_FunctionDictionary.AddFunctionRef("PERMUTATIONA","PermutationA","Math", 2, new tFunctionPermut(true));
        m_FunctionDictionary.AddFunctionRef("GEOMEAN","GeoMean","Math", -1, new tFunctionGeoMean);
        m_FunctionDictionary.AddFunctionRef("HARMEAN","HarMean","Math", -1, new tFunctionHarMean);
        m_FunctionDictionary.AddFunctionRef("TRIMMEAN","TrimMean","Math", 2, new tFunctionTrimMean);
        m_FunctionDictionary.AddFunctionRef("FREQUENCY","Frequency","Math", 2, new tFunctionFrequency);
        // Shared: true = LARGE (k-th largest), false = SMALL (k-th smallest).
        m_FunctionDictionary.AddFunctionRef("LARGE","Large","Math", 2, new tFunctionNth(true));
        m_FunctionDictionary.AddFunctionRef("SMALL","Small","Math", 2, new tFunctionNth(false));
        // MODE.SNGL / MODE.MULT / RANK.EQ / RANK.AVG rewritten on import; MODE / RANK as aliases.
        m_FunctionDictionary.AddFunctionRef("MODE_SNGL","Mode_Sngl","Math", -1, new tFunctionMode(false));
        m_FunctionDictionary.AddFunctionRef("MODE","Mode","Math", -1, new tFunctionMode(false));
        m_FunctionDictionary.AddFunctionRef("MODE_MULT","Mode_Mult","Math", -1, new tFunctionMode(true));
        m_FunctionDictionary.AddFunctionRef("RANK_EQ","Rank_Eq","Math", -1, new tFunctionRankEq(false));
        m_FunctionDictionary.AddFunctionRef("RANK","Rank","Math", -1, new tFunctionRankEq(false));
        m_FunctionDictionary.AddFunctionRef("RANK_AVG","Rank_Avg","Math", -1, new tFunctionRankEq(true));
        m_FunctionDictionary.AddFunctionRef("ATAN2","Atan2","Math", 2, new tFunctionBinaryMath(tBinaryMathKind::Atan2));
        m_FunctionDictionary.AddFunctionRef("QUOTIENT","Quotient","Math", 2, new tFunctionBinaryMath(tBinaryMathKind::Quotient));
        m_FunctionDictionary.AddFunctionRef("FACT","Fact","Math", 1, new tFunctionFact(false));
        m_FunctionDictionary.AddFunctionRef("FACTDOUBLE","FactDouble","Math", 1, new tFunctionFact(true));
        m_FunctionDictionary.AddFunctionRef("EVEN","Even","Math", 1, new tFunctionEvenOdd(true));
        m_FunctionDictionary.AddFunctionRef("ODD","Odd","Math", 1, new tFunctionEvenOdd(false));
        m_FunctionDictionary.AddFunctionRef("COMBIN","Combin","Math", 2, new tFunctionCombin(false));
        m_FunctionDictionary.AddFunctionRef("COMBINA","CombinA","Math", 2, new tFunctionCombin(true));
        m_FunctionDictionary.AddFunctionRef("BASE","Base","Math", -1, new tFunctionBase);
        m_FunctionDictionary.AddFunctionRef("DECIMAL","Decimal","Math", 2, new tFunctionDecimal);
        // Engineering bits + radix conversions (shared BitOp / EngStep / EngBaseConvert).
        m_FunctionDictionary.AddFunctionRef("BITAND","BitAnd","Math", 2, new tFunctionBitOp(tBitOpKind::And));
        m_FunctionDictionary.AddFunctionRef("BITOR","BitOr","Math", 2, new tFunctionBitOp(tBitOpKind::Or));
        m_FunctionDictionary.AddFunctionRef("BITXOR","BitXor","Math", 2, new tFunctionBitOp(tBitOpKind::Xor));
        m_FunctionDictionary.AddFunctionRef("BITLSHIFT","BitLShift","Math", 2, new tFunctionBitOp(tBitOpKind::LShift));
        m_FunctionDictionary.AddFunctionRef("BITRSHIFT","BitRShift","Math", 2, new tFunctionBitOp(tBitOpKind::RShift));
        m_FunctionDictionary.AddFunctionRef("DELTA","Delta","Math", -1, new tFunctionEngStep(tEngStepKind::Delta));
        m_FunctionDictionary.AddFunctionRef("GESTEP","GeStep","Math", -1, new tFunctionEngStep(tEngStepKind::GeStep));
        m_FunctionDictionary.AddFunctionRef("BIN2DEC","Bin2Dec","Math", 1, new tFunctionEngBaseConvert(2, 10));
        m_FunctionDictionary.AddFunctionRef("BIN2HEX","Bin2Hex","Math", -1, new tFunctionEngBaseConvert(2, 16));
        m_FunctionDictionary.AddFunctionRef("BIN2OCT","Bin2Oct","Math", -1, new tFunctionEngBaseConvert(2, 8));
        m_FunctionDictionary.AddFunctionRef("DEC2BIN","Dec2Bin","Math", -1, new tFunctionEngBaseConvert(10, 2));
        m_FunctionDictionary.AddFunctionRef("DEC2HEX","Dec2Hex","Math", -1, new tFunctionEngBaseConvert(10, 16));
        m_FunctionDictionary.AddFunctionRef("DEC2OCT","Dec2Oct","Math", -1, new tFunctionEngBaseConvert(10, 8));
        m_FunctionDictionary.AddFunctionRef("HEX2BIN","Hex2Bin","Math", -1, new tFunctionEngBaseConvert(16, 2));
        m_FunctionDictionary.AddFunctionRef("HEX2DEC","Hex2Dec","Math", 1, new tFunctionEngBaseConvert(16, 10));
        m_FunctionDictionary.AddFunctionRef("HEX2OCT","Hex2Oct","Math", -1, new tFunctionEngBaseConvert(16, 8));
        m_FunctionDictionary.AddFunctionRef("OCT2BIN","Oct2Bin","Math", -1, new tFunctionEngBaseConvert(8, 2));
        m_FunctionDictionary.AddFunctionRef("OCT2DEC","Oct2Dec","Math", 1, new tFunctionEngBaseConvert(8, 10));
        m_FunctionDictionary.AddFunctionRef("OCT2HEX","Oct2Hex","Math", -1, new tFunctionEngBaseConvert(8, 16));
        m_FunctionDictionary.AddFunctionRef("ROMAN","Roman","Math", -1, new tFunctionRoman);
        m_FunctionDictionary.AddFunctionRef("ARABIC","Arabic","Math", 1, new tFunctionArabic);
        m_FunctionDictionary.AddFunctionRef("MULTINOMIAL","Multinomial","Math", -1, new tFunctionMultinomial);
        m_FunctionDictionary.AddFunctionRef("SERIESSUM","SeriesSum","Math", 4, new tFunctionSeriesSum);
        m_FunctionDictionary.AddFunctionRef("PERCENTOF","PercentOf","Math", 2, new tFunctionPercentOf);
        // Shared unary math (constructor selects the operation).
        m_FunctionDictionary.AddFunctionRef("SIN","Sin","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sin));
        m_FunctionDictionary.AddFunctionRef("COS","cos","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Cos));
        m_FunctionDictionary.AddFunctionRef("TAN","Tan","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Tan));
        m_FunctionDictionary.AddFunctionRef("ACOS","Acos","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Acos));
        m_FunctionDictionary.AddFunctionRef("ASIN","Asin","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Asin));
        m_FunctionDictionary.AddFunctionRef("ATAN","Atan","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Atan));
        m_FunctionDictionary.AddFunctionRef("SQRT","Sqrt","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sqrt));
        m_FunctionDictionary.AddFunctionRef("LOG","Log","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Log));
        m_FunctionDictionary.AddFunctionRef("LOG10","Log10","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Log10));
        // Legacy alias (older sker workbooks used LOG_10).
        m_FunctionDictionary.AddFunctionRef("LOG_10","Log10","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Log10));
        m_FunctionDictionary.AddFunctionRef("EXP","Exp","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Exp));
        m_FunctionDictionary.AddFunctionRef("LN","Ln","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Ln));
        m_FunctionDictionary.AddFunctionRef("PI","Pi","Math", 0, new tFunctionPi);
        m_FunctionDictionary.AddFunctionRef("RADIANS","Radians","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Radians));
        m_FunctionDictionary.AddFunctionRef("DEGREES","Degrees","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Degrees));
        m_FunctionDictionary.AddFunctionRef("COSH","Cosh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Cosh));
        m_FunctionDictionary.AddFunctionRef("SINH","Sinh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sinh));
        m_FunctionDictionary.AddFunctionRef("TANH","Tanh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Tanh));
        m_FunctionDictionary.AddFunctionRef("ACOSH","Acosh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Acosh));
        m_FunctionDictionary.AddFunctionRef("ASINH","Asinh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Asinh));
        m_FunctionDictionary.AddFunctionRef("ATANH","Atanh","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Atanh));
        m_FunctionDictionary.AddFunctionRef("COT","Cot","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Cot));
        m_FunctionDictionary.AddFunctionRef("COTH","Coth","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Coth));
        m_FunctionDictionary.AddFunctionRef("CSC","Csc","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Csc));
        m_FunctionDictionary.AddFunctionRef("CSCH","Csch","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Csch));
        m_FunctionDictionary.AddFunctionRef("SEC","Sec","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sec));
        m_FunctionDictionary.AddFunctionRef("SECH","Sech","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sech));
        m_FunctionDictionary.AddFunctionRef("ACOT","Acot","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Acot));
        m_FunctionDictionary.AddFunctionRef("ACOTH","Acoth","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Acoth));
        m_FunctionDictionary.AddFunctionRef("SIGN","Sign","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::Sign));
        m_FunctionDictionary.AddFunctionRef("SQRTPI","SqrtPi","Math", 1, new tFunctionUnaryMath(tUnaryMathKind::SqrtPi));
        m_FunctionDictionary.AddFunctionRef("MOD","Mod","Math", 2, new tFunctionMod);
        m_FunctionDictionary.AddFunctionRef("POWER","Power","Math", 2, new tFunctionPower);
        m_FunctionDictionary.AddFunctionRef("PRODUCT","Product","Math", -1, new tFunctionProduct);
        m_FunctionDictionary.AddFunctionRef("SUMPRODUCT","SumProduct","Math", -1, new tFunctionSumProduct);
        m_FunctionDictionary.AddFunctionRef("SUMSQ","SumSq","Math", -1, new tFunctionSumSq);
        m_FunctionDictionary.AddFunctionRef("SUMX2MY2","SumX2MY2","Math", 2, new tFunctionSumX(tSumXKind::X2MinusY2));
        m_FunctionDictionary.AddFunctionRef("SUMX2PY2","SumX2PY2","Math", 2, new tFunctionSumX(tSumXKind::X2PlusY2));
        m_FunctionDictionary.AddFunctionRef("SUMXMY2","SumXMY2","Math", 2, new tFunctionSumX(tSumXKind::XMinusY2));
        m_FunctionDictionary.AddFunctionRef("RAND","Rand","Math", 0, new tFunctionRand,tVolatile::t_All);
        m_FunctionDictionary.AddFunctionRef("RANDBETWEEN","RandBetween","Math", 2, new tFunctionRandBetween,tVolatile::t_All);
        m_FunctionDictionary.AddFunctionRef("RANDARRAY","RandArray","Array", -1, new tFunctionRandArray, tVolatile::t_All);
        
        // SPREADSHEET ========================================================
        m_FunctionDictionary.AddFunctionRef("COUNT","Count","Sheet", -1, new tFunctionCount);
        m_FunctionDictionary.AddFunctionRef("COUNTA","CountA","Sheet", -1, new tFunctionCountA);
        m_FunctionDictionary.AddFunctionRef("COUNTBLANK","CountBlank","Sheet", -1, new tFunctionCountBlank);
        m_FunctionDictionary.AddFunctionRef("COUNTIF","CountIf","Sheet", 2, new tFunctionCountIf);
        m_FunctionDictionary.AddFunctionRef("COUNTIFS","CountIfs","Sheet", -1, new tFunctionCountIfs);
        m_FunctionDictionary.AddFunctionRef("SUMIFS","SumIfs","Sheet", -1, new tFunctionAggIfs(tAggIfsKind::Sum));
        m_FunctionDictionary.AddFunctionRef("MAXIFS","MaxIfs","Sheet", -1, new tFunctionAggIfs(tAggIfsKind::Max));
        m_FunctionDictionary.AddFunctionRef("MINIFS","MinIfs","Sheet", -1, new tFunctionAggIfs(tAggIfsKind::Min));
        m_FunctionDictionary.AddFunctionRef("AVERAGEIF","AverageIf","Sheet", 3, new tFunctionAverageIf);
        m_FunctionDictionary.AddFunctionRef("AVERAGEIFS","AverageIfs","Sheet", -1, new tFunctionAverageIfs);
        m_FunctionDictionary.AddFunctionRef("SUMIF","SumIf","Sheet", 3, new tFunctionSumIf);
        m_FunctionDictionary.AddFunctionRef("INDEX","Index","Sheet", 3, new tFunctionIndex);
        m_FunctionDictionary.AddFunctionRef("OFFSET","Offset","Sheet", -1, new tFunctionOffset);
        // Dynamic arrays (spill): return an in-memory t_Array, materialized by tCell::InternalCalculation.
        m_FunctionDictionary.AddFunctionRef("SEQUENCE","Sequence","Array", -1, new tFunctionSequence);
        m_FunctionDictionary.AddFunctionRef("SORT","Sort","Array", -1, new tFunctionSort);
        m_FunctionDictionary.AddFunctionRef("UNIQUE","Unique","Array", -1, new tFunctionUnique);
        m_FunctionDictionary.AddFunctionRef("FILTER","Filter","Array", -1, new tFunctionFilter);
        m_FunctionDictionary.AddFunctionRef("TRANSPOSE","Transpose","Array", 1, new tFunctionTranspose);
        m_FunctionDictionary.AddFunctionRef("MMULT","MMult","Array", 2, new tFunctionMMult);
        m_FunctionDictionary.AddFunctionRef("MDETERM","MDeterm","Array", 1, new tFunctionMDeterm);
        m_FunctionDictionary.AddFunctionRef("MINVERSE","MInverse","Array", 1, new tFunctionMInverse);
        m_FunctionDictionary.AddFunctionRef("TOCOL","ToCol","Array", -1, new tFunctionToColRow(true));
        m_FunctionDictionary.AddFunctionRef("TOROW","ToRow","Array", -1, new tFunctionToColRow(false));
        m_FunctionDictionary.AddFunctionRef("CHOOSECOLS","ChooseCols","Array", -1, new tFunctionChooseDim(true));
        m_FunctionDictionary.AddFunctionRef("CHOOSEROWS","ChooseRows","Array", -1, new tFunctionChooseDim(false));
        m_FunctionDictionary.AddFunctionRef("EXPAND","Expand","Array", -1, new tFunctionExpand);
        m_FunctionDictionary.AddFunctionRef("WRAPROWS","WrapRows","Array", -1, new tFunctionWrap(true));
        m_FunctionDictionary.AddFunctionRef("WRAPCOLS","WrapCols","Array", -1, new tFunctionWrap(false));
        m_FunctionDictionary.AddFunctionRef("TAKE","Take","Array", -1, new tFunctionTakeDrop(false));
        m_FunctionDictionary.AddFunctionRef("DROP","Drop","Array", -1, new tFunctionTakeDrop(true));
        m_FunctionDictionary.AddFunctionRef("HSTACK","HStack","Array", -1, new tFunctionStack(true));
        m_FunctionDictionary.AddFunctionRef("VSTACK","VStack","Array", -1, new tFunctionStack(false));
        m_FunctionDictionary.AddFunctionRef("SORTBY","SortBy","Array", -1, new tFunctionSortBy);
        m_FunctionDictionary.AddFunctionRef("MAP","Map","Array", -1, new tFunctionMap);
        m_FunctionDictionary.AddFunctionRef("REDUCE","Reduce","Array", -1, new tFunctionReduce);
        m_FunctionDictionary.AddFunctionRef("SCAN","Scan","Array", -1, new tFunctionScan);
        m_FunctionDictionary.AddFunctionRef("BYROW","ByRow","Array", 2, new tFunctionByRowCol(true));
        m_FunctionDictionary.AddFunctionRef("BYCOL","ByCol","Array", 2, new tFunctionByRowCol(false));
        m_FunctionDictionary.AddFunctionRef("MAKEARRAY","MakeArray","Array", 3, new tFunctionMakeArray);
        m_FunctionDictionary.AddFunctionRef("MUNIT","MUnit","Array", 1, new tFunctionMUnit);
        m_FunctionDictionary.AddFunctionRef("TRIMRANGE","TrimRange","Array", -1, new tFunctionTrimRange);
        m_FunctionDictionary.AddFunctionRef("DATARANGE","DataRange","Sheet", -1, new tFunctionDataRange);
        m_FunctionDictionary.AddFunctionRef("JSON","Json","Sheet", -1, new tFunctionJson);
        m_FunctionDictionary.AddFunctionRef("LOOKUP","Lookup","Sheet", -1, new tFunctionLookup);
        m_FunctionDictionary.AddFunctionRef("VLOOKUP","VLookup","Sheet", -1, new tFunctionVLookup);
        m_FunctionDictionary.AddFunctionRef("HLOOKUP","HLookup","Sheet", -1, new tFunctionHLookup);
        m_FunctionDictionary.AddFunctionRef("XLOOKUP","XLookup","Sheet", -1, new tFunctionXLookup);
        m_FunctionDictionary.AddFunctionRef("XMATCH","XMatch","Sheet", -1, new tFunctionXMatch);
        m_FunctionDictionary.AddFunctionRef("MATCH","Match","Sheet", 3, new tFunctionMatch);
        m_FunctionDictionary.AddFunctionRef("ROW","Row","Sheet", 1, new tFunctionRow,tVolatile::t_Row);
        m_FunctionDictionary.AddFunctionRef("COLUMN","Column","Sheet", 1, new tFunctionColumn,tVolatile::t_Col);
        m_FunctionDictionary.AddFunctionRef("ROWS","Rows","Sheet", 1, new tFunctionRows,tVolatile::t_Row);
        m_FunctionDictionary.AddFunctionRef("COLUMNS","Columns","Sheet", 1, new tFunctionColumns,tVolatile::t_Col);
        m_FunctionDictionary.AddFunctionRef("ADDRESS","Address","Sheet", 5, new tFunctionAddress);
        m_FunctionDictionary.AddFunctionRef("INDIRECT","Indirect","Sheet", -1, new tFunctionIndirect, tVolatile::t_All);
        m_FunctionDictionary.AddFunctionRef("SUBTOTAL","Subtotal","Sheet", -1, new tFunctionSubtotal);
        m_FunctionDictionary.AddFunctionRef("AGGREGATE","Aggregate","Sheet", -1, new tFunctionAggregate);
        
        // TEXT ==============================================================
        m_FunctionDictionary.AddFunctionRef("CONCAT","Concat","Text", -1, new tFunctionConcat);
        m_FunctionDictionary.AddFunctionRef("CONCATENATE","Concatenate","Text", -1, new tFunctionConcat);
        m_FunctionDictionary.AddFunctionRef("LEFT","Left","Text", -1, new tFunctionLeft);   // 1 or 2 args (num_chars optional)
        m_FunctionDictionary.AddFunctionRef("RIGHT","Right","Text", -1, new tFunctionRight); // 1 or 2 args (num_chars optional)
        m_FunctionDictionary.AddFunctionRef("MID","Mid","Text", 3, new tFunctionMid);
        m_FunctionDictionary.AddFunctionRef("LEN","Len","Text", 1, new tFunctionLen);
        m_FunctionDictionary.AddFunctionRef("FIND","Find","Text", 2, new tFunctionFind);
        m_FunctionDictionary.AddFunctionRef("EXACT","Exact","Text", 2, new tFunctionExact);
        m_FunctionDictionary.AddFunctionRef("SEARCH","Search","Text", 2, new tFunctionSearch);
        m_FunctionDictionary.AddFunctionRef("REPLACE","Replace","Text", 4, new tFunctionReplace);
        m_FunctionDictionary.AddFunctionRef("SUBSTITUTE","Substitute","Text", 3, new tFunctionSubstitute);
        m_FunctionDictionary.AddFunctionRef("UPPER","Upper","Text", 1, new tFunctionUpper);
        m_FunctionDictionary.AddFunctionRef("LOWER","Lower","Text", 1, new tFunctionLower);
        m_FunctionDictionary.AddFunctionRef("PROPER","Proper","Text", 1, new tFunctionProper);
        m_FunctionDictionary.AddFunctionRef("TRIM","Trim","Text", 1, new tFunctionTrim);
        m_FunctionDictionary.AddFunctionRef("CLEAN","Clean","Text", 1, new tFunctionClean);
        m_FunctionDictionary.AddFunctionRef("REPT","Rept","Text", 2, new tFunctionRept);
        m_FunctionDictionary.AddFunctionRef("TEXT","Text","Text", 2, new tFunctionText);
        m_FunctionDictionary.AddFunctionRef("T","T","Text", 1, new tFunctionT);
        m_FunctionDictionary.AddFunctionRef("FIXED","Fixed","Text", -1, new tFunctionFixed);
        m_FunctionDictionary.AddFunctionRef("DOLLAR","Dollar","Text", -1, new tFunctionDollar);
        m_FunctionDictionary.AddFunctionRef("VALUETOTEXT","ValueToText","Text", -1, new tFunctionValueToText);
        m_FunctionDictionary.AddFunctionRef("ARRAYTOTEXT","ArrayToText","Text", -1, new tFunctionArrayToText);
        // Shared: true = CHAR, false = CODE (first-byte code, inverse of CHAR for 1..255).
        m_FunctionDictionary.AddFunctionRef("CHAR","Char","Text", 1, new tFunctionCharCode(true));
        m_FunctionDictionary.AddFunctionRef("CODE","Code","Text", 1, new tFunctionCharCode(false));
        // Shared: true = UNICHAR, false = UNICODE (first UTF-8 code point).
        m_FunctionDictionary.AddFunctionRef("UNICHAR","UniChar","Text", 1, new tFunctionUniCharCode(true));
        m_FunctionDictionary.AddFunctionRef("UNICODE","Unicode","Text", 1, new tFunctionUniCharCode(false));
        m_FunctionDictionary.AddFunctionRef("VALUE","Value","Text", 1, new tFunctionValue);
        m_FunctionDictionary.AddFunctionRef("NUMBERVALUE","NumberValue","Text", -1, new tFunctionNumberValue);
        m_FunctionDictionary.AddFunctionRef("TEXTAFTER","TextAfter","Text", -1, new tFunctionTextAfterBefore(true));
        m_FunctionDictionary.AddFunctionRef("TEXTBEFORE","TextBefore","Text", -1, new tFunctionTextAfterBefore(false));
        m_FunctionDictionary.AddFunctionRef("TEXTJOIN","TextJoin","Text", -1, new tFunctionTextJoin);
        m_FunctionDictionary.AddFunctionRef("TEXTSPLIT","TextSplit","Text", -1, new tFunctionTextSplit);
        // REGEX* use ECMAScript (std::regex), not Excel PCRE2 — advanced patterns may differ.
        m_FunctionDictionary.AddFunctionRef("REGEXTEST","RegexTest","Text", -1, new tFunctionRegexTest);
        m_FunctionDictionary.AddFunctionRef("REGEXREPLACE","RegexReplace","Text", -1, new tFunctionRegexReplace);
        m_FunctionDictionary.AddFunctionRef("REGEXEXTRACT","RegexExtract","Text", -1, new tFunctionRegexExtract);
        
        // LOGICAL ===========================================================
        // IF / IFERROR: Excel allows IF(cond, a [, b]) and IFERROR(val, err); arity is in formula bytecode (Extra), not dictionary.
        m_FunctionDictionary.AddFunctionRef("IF","If","logical", -1, new tFunctionIf);  // 2 or 3
        // Shared: false = IFERROR (any error), true = IFNA (#N/A only).
        m_FunctionDictionary.AddFunctionRef("IFERROR","IfError","logical", -1, new tFunctionIfError(false));
        m_FunctionDictionary.AddFunctionRef("IFNA","IfNa","logical", 2, new tFunctionIfError(true));
        m_FunctionDictionary.AddFunctionRef("IFS","Ifs","logical", -1, new tFunctionIfs);
        m_FunctionDictionary.AddFunctionRef("SWITCH","Switch","logical", -1, new tFunctionSwitch);
        m_FunctionDictionary.AddFunctionRef("CHOOSE","Choose","logical", -1, new tFunctionChoose);
        m_FunctionDictionary.AddFunctionRef("ISBLANK","IsBlank","logical", 1, new tFunctionIsBlank);
        // Shared IS* type predicates (constructor selects the Excel semantics).
        m_FunctionDictionary.AddFunctionRef("ISNA","IsNa","logical", 1, new tFunctionIsType(tIsTypeKind::IsNa));
        m_FunctionDictionary.AddFunctionRef("ISERR","IsErr","logical", 1, new tFunctionIsType(tIsTypeKind::IsErr));
        m_FunctionDictionary.AddFunctionRef("ISERROR","IsError","logical", 1, new tFunctionIsType(tIsTypeKind::IsError));
        m_FunctionDictionary.AddFunctionRef("ISNUMBER","IsNumber","logical", 1, new tFunctionIsType(tIsTypeKind::IsNumber));
        m_FunctionDictionary.AddFunctionRef("ISLOGICAL","IsLogical","logical", 1, new tFunctionIsType(tIsTypeKind::IsLogical));
        m_FunctionDictionary.AddFunctionRef("ISTEXT","IsText","logical", 1, new tFunctionIsType(tIsTypeKind::IsText));
        m_FunctionDictionary.AddFunctionRef("ISNONTEXT","IsNonText","logical", 1, new tFunctionIsType(tIsTypeKind::IsNonText));
        m_FunctionDictionary.AddFunctionRef("ISREF","IsRef","logical", 1, new tFunctionIsType(tIsTypeKind::IsRef));
        m_FunctionDictionary.AddFunctionRef("ISFORMULA","IsFormula","logical", 1, new tFunctionIsType(tIsTypeKind::IsFormula));
        // Shared: true = ISEVEN, false = ISODD (truncate toward zero, then parity).
        m_FunctionDictionary.AddFunctionRef("ISEVEN","IsEven","logical", 1, new tFunctionIsParity(true));
        m_FunctionDictionary.AddFunctionRef("ISODD","IsOdd","logical", 1, new tFunctionIsParity(false));
        m_FunctionDictionary.AddFunctionRef("ISOMITTED","IsOmitted","logical", 1, new tFunctionIsOmitted);
        m_FunctionDictionary.AddFunctionRef("N","N","logical", 1, new tFunctionN);
        m_FunctionDictionary.AddFunctionRef("TYPE","Type","logical", 1, new tFunctionType);
        // Excel ERROR.TYPE → ERROR_TYPE on import (dotted rewrite).
        m_FunctionDictionary.AddFunctionRef("ERROR_TYPE","ErrorType","logical", 1, new tFunctionErrorType);
        m_FunctionDictionary.AddFunctionRef("FORMULATEXT","FormulaText","logical", 1, new tFunctionFormulaText);
        m_FunctionDictionary.AddFunctionRef("CELL","Cell","logical", -1, new tFunctionCellInfo);
        m_FunctionDictionary.AddFunctionRef("INFO","Info","logical", 1, new tFunctionInfo);
        m_FunctionDictionary.AddFunctionRef("SHEET","Sheet","logical", -1, new tFunctionSheetNum);
        m_FunctionDictionary.AddFunctionRef("SHEETS","Sheets","logical", -1, new tFunctionSheets);
        m_FunctionDictionary.AddFunctionRef("NA","Na","logical", 0, new tFunctionNa);
        // Shared implementation: constructor selects the constant returned by TRUE()/FALSE().
        m_FunctionDictionary.AddFunctionRef("TRUE","True","logical", 0, new tFunctionTrueFalse(true));
        m_FunctionDictionary.AddFunctionRef("FALSE","False","logical", 0, new tFunctionTrueFalse(false));
        m_FunctionDictionary.AddFunctionRef("AND","And","logical", -1, new tFunctionAnd);
        m_FunctionDictionary.AddFunctionRef("OR","Or","logical", -1, new tFunctionOr);
        m_FunctionDictionary.AddFunctionRef("XOR","Xor","logical", -1, new tFunctionXor);
        m_FunctionDictionary.AddFunctionRef("NOT","Not","logical", 1, new tFunctionNot);
        // LET is compiled to dedicated scope opcodes (LetBeginScope/LetBind/LetVarRef/LetEndScope), so
        // tFunctionLet::Call is never invoked at runtime. It is registered only so the compiler treats
        // "LET" as a known function name (kept verbatim in the formula key, not resolved as a #NAME?).
        m_FunctionDictionary.AddFunctionRef("LET","Let","logical", -1, new tFunctionLet);

        // DATE ==============================================================
        m_FunctionDictionary.AddFunctionRef("TODAY","Today","Date",0, new tFunctionToday);
        m_FunctionDictionary.AddFunctionRef("NOW","Now","Date",0, new tFunctionNow);
        m_FunctionDictionary.AddFunctionRef("DATE","Date","Date",3, new tFunctionDateCreate);
        m_FunctionDictionary.AddFunctionRef("DATEVALUE","DateValue","Date",1, new tFunctionDateValue);
        m_FunctionDictionary.AddFunctionRef("DAYS","Days","Date",2, new tFunctionDays);
        m_FunctionDictionary.AddFunctionRef("ISOWEEKNUM","IsoWeekNum","Date",1, new tFunctionIsoWeekNum);
        m_FunctionDictionary.AddFunctionRef("TIMEVALUE","TimeValue","Date",1, new tFunctionTimeValue);
        m_FunctionDictionary.AddFunctionRef("DATEDIF","DateDif","Date",3, new tFunctionDateDif);
        m_FunctionDictionary.AddFunctionRef("YEARFRAC","YearFrac","Date", -1, new tFunctionYearFrac);
        m_FunctionDictionary.AddFunctionRef("DAYS360","Days360","Date", -1, new tFunctionDays360);
        m_FunctionDictionary.AddFunctionRef("NETWORKDAYS","NetworkDays","Date", -1, new tFunctionNetworkDays);
        m_FunctionDictionary.AddFunctionRef("NETWORKDAYS_INTL","NetworkDaysIntl","Date", -1, new tFunctionNetworkDaysIntl);
        m_FunctionDictionary.AddFunctionRef("WORKDAY","WorkDay","Date", -1, new tFunctionWorkDay(false));
        // Excel WORKDAY.INTL → WORKDAY_INTL on import (dotted rewrite).
        m_FunctionDictionary.AddFunctionRef("WORKDAY_INTL","WorkDayIntl","Date", -1, new tFunctionWorkDay(true));
        m_FunctionDictionary.AddFunctionRef("YEAR","Year","Date" ,1, new tFunctionYear);
        m_FunctionDictionary.AddFunctionRef("MONTH","Month","Date",1, new tFunctionMonth);
        m_FunctionDictionary.AddFunctionRef("DAY","Day","Date", 1, new tFunctionDay);
        m_FunctionDictionary.AddFunctionRef("WEEKDAY","Day of week","Date",2, new tFunctionWeekDay);
        m_FunctionDictionary.AddFunctionRef("WEEKNUM","Week Number","Date",1, new tFunctionWeekNum);
        m_FunctionDictionary.AddFunctionRef("TIME","Time","Date",3, new tFunctionTime);
        m_FunctionDictionary.AddFunctionRef("EDATE","Edate","Date",2, new tFunctionEdate);
        m_FunctionDictionary.AddFunctionRef("EOMONTH","Eomonth","Date",2, new tFunctionEomonth);
        m_FunctionDictionary.AddFunctionRef("HOUR","Hour","Date",1, new tFunctionHour);
        m_FunctionDictionary.AddFunctionRef("MINUTE","Minute","Date",1, new tFunctionMinute);
        m_FunctionDictionary.AddFunctionRef("SECOND","Second","Date",1, new tFunctionSecond);

        // FINANCIAL ============================================================
        m_FunctionDictionary.AddFunctionRef("PMT","Pmt","Financial", 5, new tFunctionPmt);
        m_FunctionDictionary.AddFunctionRef("IPMT","Ipmt","Financial", 6, new tFunctionIpmt);
        m_FunctionDictionary.AddFunctionRef("PPMT","Ppmt","Financial", 6, new tFunctionPpmt);
        m_FunctionDictionary.AddFunctionRef("PV","Pv","Financial", 5, new tFunctionPv);
        m_FunctionDictionary.AddFunctionRef("FV","Fv","Financial", 5, new tFunctionFv);
        m_FunctionDictionary.AddFunctionRef("RATE","Rate","Financial", 6, new tFunctionRate);
        m_FunctionDictionary.AddFunctionRef("NPER","Nper","Financial", 5, new tFunctionNper);
        m_FunctionDictionary.AddFunctionRef("NPV","Npv","Financial", -1, new tFunctionNpv);
        m_FunctionDictionary.AddFunctionRef("IRR","Irr","Financial", 2, new tFunctionIrr);
        m_FunctionDictionary.AddFunctionRef("MIRR","Mirr","Financial", 3, new tFunctionMirr);
        m_FunctionDictionary.AddFunctionRef("DB","Db","Financial", 5, new tFunctionDb);
        m_FunctionDictionary.AddFunctionRef("DDB","Ddb","Financial", 5, new tFunctionDdb);
        m_FunctionDictionary.AddFunctionRef("SLN","Sln","Financial", 3, new tFunctionSln);
        m_FunctionDictionary.AddFunctionRef("SYD","Syd","Financial", 4, new tFunctionSyd);
        m_FunctionDictionary.AddFunctionRef("CUMIPMT","Cumipmt","Financial", 6, new tFunctionCumipmt);
        m_FunctionDictionary.AddFunctionRef("CUMPRINC","Cumprinc","Financial", 6, new tFunctionCumprinc);
        m_FunctionDictionary.AddFunctionRef("EFFECT","Effect","Financial", 2, new tFunctionEffect);
        m_FunctionDictionary.AddFunctionRef("NOMINAL","Nominal","Financial", 2, new tFunctionNominal);
        m_FunctionDictionary.AddFunctionRef("XIRR","Xirr","Financial", 3, new tFunctionXirr);
        m_FunctionDictionary.AddFunctionRef("XNPV","Xnpv","Financial", -1, new tFunctionXnpv);
        m_FunctionDictionary.AddFunctionRef("VDB","Vdb","Financial", 7, new tFunctionVdb);
        m_FunctionDictionary.AddFunctionRef("FVSCHEDULE","Fvschedule","Financial", 2, new tFunctionFvschedule);
        m_FunctionDictionary.AddFunctionRef("PDURATION","Pduration","Financial", 3, new tFunctionPduration);
        m_FunctionDictionary.AddFunctionRef("RRI","Rri","Financial", 3, new tFunctionRri);

	}

	tSpreadSheetContainer::~tSpreadSheetContainer() {
		Clear();
	}

	void tSpreadSheetContainer::Clear() {
		if (m_VectorWorkBook.size() > 0) {
			for (auto wWorkBookRef : m_VectorWorkBook) {
				m_AllocatorWorkBook.Delete(wWorkBookRef);
			}
			m_VectorWorkBook.clear();
		}
        m_ActiveWorkBook=nullptr;
	}

	//=========================================================================
	//! Sorted Workbook by operator <
	class tComparatorWorkBookInsert {
	private:
		tSpreadSheetContainer::tAllocatorWorkBook* m_Allocator;
		tWorkBook* m_WorkBookSearch;
	public:
		tComparatorWorkBookInsert(tSpreadSheetContainer::tAllocatorWorkBook* sAllocator, tWorkBook* sWorkBookSearch) { m_Allocator = sAllocator;  m_WorkBookSearch = sWorkBookSearch; }
		/// @brief      Operator() compare with ttWorkbook < operator.
		/// @param[in]  sE2 tRange*
		/// @param[in]  sE1 tRange*
		SkInline tWorkBook* Get(tAllocatorRef sIndex) {
			if (sIndex == 0) return(m_WorkBookSearch);
			return((*m_Allocator)(sIndex));
		}

		SkInline bool operator()(tAllocatorRef sE1, tAllocatorRef sE2) {
			tWorkBook* wWorkBook1 = Get(sE1);
			tWorkBook* wWorkBook2 = Get(sE2);
			return(*wWorkBook1 < *wWorkBook2);
		}
	};

    class tComparatorWorkBook {
    private:
        tSpreadSheetContainer::tAllocatorWorkBook* m_Allocator;
    public:
        tComparatorWorkBook(tSpreadSheetContainer::tAllocatorWorkBook* sAllocator) { m_Allocator = sAllocator; }
        /// @brief      Operator() compare with ttWorkbook < operator.
        /// @param[in]  sE2 tRange*
        /// @param[in]  sE1 tRange*
        SkInline tWorkBook* Get(tAllocatorRef sIndex) {
            return((*m_Allocator)(sIndex));
        }

        SkInline bool operator()(tAllocatorRef sE1, tAllocatorRef sE2) {
            tWorkBook* wWorkBook1 = Get(sE1);
            tWorkBook* wWorkBook2 = Get(sE2);
            return(*wWorkBook1 < *wWorkBook2);
        }
    };

	tSpreadSheetContainer::tVectorWorkBook::iterator tSpreadSheetContainer::FindWorkBookByIterator(tString sUri) {
		tVectorWorkBook::iterator wWhere;
		tWorkBook wWorkBook;
		wWorkBook.Set(0,sUri); // Set 0 to m_WorkBookSearch
		wWhere = std::lower_bound(m_VectorWorkBook.begin(), m_VectorWorkBook.end(), 0, tComparatorWorkBookInsert(&m_AllocatorWorkBook, &wWorkBook));
       
        return(wWhere);
	}

    void tSpreadSheetContainer::ClearUndoRedo() {
        tApplication::Instance()->ClearUndoRedo();
        for(tVectorWorkBook::iterator wIterator=m_VectorWorkBook.begin();wIterator<m_VectorWorkBook.end();wIterator++) {
            tWorkBook* wWorkBook = m_AllocatorWorkBook(*wIterator);
            wWorkBook->ClearUndoRedo();
        }
    }

	tBool tSpreadSheetContainer::ActiveWorkBook(tString sUri) {
		tVectorWorkBook::iterator wWhere=FindWorkBookByIterator(sUri);
		if (wWhere != m_VectorWorkBook.end()) {
			tWorkBook* wResult = m_AllocatorWorkBook(*wWhere);
			if (wResult->Uri() == sUri) {
				m_ActiveWorkBook = wResult;
				return(true);
			}
		}
		return(false);
	}

	void  tSpreadSheetContainer::ActiveWorkBook(tWorkBook* sWorkBook) {
		m_ActiveWorkBook = sWorkBook;
    }

    tFormatApi* tSpreadSheetContainer::FormatApi() {
        return(m_FormatApi);
    }
    
    void tSpreadSheetContainer::FormatApi(tFormatApi* sFormatApi) {
        m_FormatApi=sFormatApi;
    }


	tWorkBook* tSpreadSheetContainer::ActiveWorkBook() {
        return(m_ActiveWorkBook);
	}

	tWorkBook* tSpreadSheetContainer::WorkBook(tAllocatorRef sRef) {
		return(m_AllocatorWorkBook(sRef));
	}

	tWorkBook* tSpreadSheetContainer::AddWorkBook(tString sUri) {
		tVectorWorkBook::iterator wWhere = FindWorkBookByIterator(sUri);
		if (wWhere != m_VectorWorkBook.end()) {
			// If exist then return WorkBook
			m_ActiveWorkBook = m_AllocatorWorkBook(*wWhere);
            if (m_ActiveWorkBook->Uri() == sUri) { return(m_ActiveWorkBook); }
		}
		tAllocatorRef wRef;
		tie(wRef, m_ActiveWorkBook) = m_AllocatorWorkBook.Alloc();
		m_ActiveWorkBook->Set(wRef, sUri);
        tWorkBook wWorkBook;
        wWorkBook.Set(0,sUri); // Set 0 to m_WorkBookSearch
        m_VectorWorkBook.insert(wWhere,m_ActiveWorkBook->AllocatorRef());
        
#ifdef debugworkbook
        cout << "Add WorkBook " << sUri <<  ":" << wRef << endl;
#endif
        return(m_ActiveWorkBook);
	}

	tWorkBook* tSpreadSheetContainer::FindWorkBook(tString sUri) {
		tVectorWorkBook::iterator wWhere = FindWorkBookByIterator(sUri);
		if (wWhere != m_VectorWorkBook.end()) {
            tWorkBook* wWorkBook=m_AllocatorWorkBook(*wWhere);
            if ((wWorkBook->Uri()==sUri) && (wWorkBook->AllocatorRef()!=0)) {
                m_ActiveWorkBook = wWorkBook;
                return(m_ActiveWorkBook);
            }
		}
		return(nullptr);
	}

    tBool tSpreadSheetContainer::DeleteWorkBook(tString sUri) {
        tVectorWorkBook::iterator wWhere = FindWorkBookByIterator(sUri);

#ifdef debugworkbook
        cout << "Before Delete " << endl;
        tVectorWorkBook::iterator wIterator;
        for(wIterator = m_VectorWorkBook.begin(); wIterator!=m_VectorWorkBook.end();wIterator++) {
            tWorkBook* wWorkBook=m_AllocatorWorkBook(*wIterator);
            cout << wWorkBook->Uri() << ":" << *wIterator << endl;
        }
#endif
        if (wWhere != m_VectorWorkBook.end()) {

#ifdef debugworkbook
            cout << "Delete WorkBook " << *wWhere << ":";
            tWorkBook* wWorkBook=m_AllocatorWorkBook(*wWhere);
            cout << wWorkBook->Uri() << endl;
#endif
            if (m_ActiveWorkBook != nullptr
                && m_ActiveWorkBook->AllocatorRef() == *wWhere) {
                m_ActiveWorkBook = nullptr;
            }
            // Erase in allocator ============================================
            m_AllocatorWorkBook.Delete((*wWhere));
            
            // Erase in Vector
            m_VectorWorkBook.erase(wWhere);

            // Last workbook gone: drop orphan table-style refs still held in the shared pool.
            if (m_VectorWorkBook.empty()) {
                if (tFormatApi* wFormatApi = FormatApi(); wFormatApi != nullptr) {
                    wFormatApi->Clear();
                }
            }
           
            return(true);
        }
        return(false);
    }

    tBool tSpreadSheetContainer::RenameWorkBook(tString sUri,tString sToUri) {
        tVectorWorkBook::iterator wWhere = FindWorkBookByIterator(sUri);
        if (wWhere != m_VectorWorkBook.end()) {
            tWorkBook* wWorkBook=m_AllocatorWorkBook(*wWhere);
            wWorkBook->Uri(sToUri);
        
            std::sort(m_VectorWorkBook.begin(),m_VectorWorkBook.end(),tComparatorWorkBook(&m_AllocatorWorkBook));
            return(true);
        }
        return(false);
    }

    void tSpreadSheetContainer::JsonWorkBooksList(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("list");
        sWriter->StartArray();
        for(tVectorWorkBook::iterator wIterator=m_VectorWorkBook.begin();wIterator<m_VectorWorkBook.end();wIterator++) {
            tWorkBook* wWorkBook = m_AllocatorWorkBook(*wIterator);
            sWriter->String(wWorkBook->Uri().c_str());
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tSpreadSheetContainer::WorkBooksList(tVectorWorkBookClass& sVectorWorkBookClass) {
        for(tVectorWorkBook::iterator wIterator=m_VectorWorkBook.begin();wIterator<m_VectorWorkBook.end();wIterator++) {
            tWorkBook* wWorkBook = m_AllocatorWorkBook(*wIterator);
            sVectorWorkBookClass.push_back(wWorkBook);
        }
    }

	tFunctionDictionary* tSpreadSheetContainer::FunctionDictionary() {
		return(&m_FunctionDictionary);
	}

	// Lemon interface
	tLemonInterface* tSpreadSheetContainer::LemonInterface() {
		return(&m_LemonInterface);
	}


	tSharedFormulaPool* tSpreadSheetContainer::SharedFormulaPool() {
		return(&m_SharedFormulaPool);
	};


	tIndex tSpreadSheetContainer::NbSharedFormula() {
		return(tIndex(m_SharedFormulaPool.Size()));
	}
            
    tBool tSpreadSheetContainer::IsOnCellChange() {
        return(m_OnCellChange!=nullptr);
    }
    // Check
#ifdef checksp
    /// @brief Check.
    void tSpreadSheetContainer::Check() {
        for(tVectorWorkBook::iterator wIterator=m_VectorWorkBook.begin();wIterator<m_VectorWorkBook.end();wIterator++) {
            m_AllocatorWorkBook(*wIterator)->Check();
        }
    }
#endif
#ifdef checkfo
    void tSpreadSheetContainer::CheckFormatUndo(tUndo* sUndo) {
        if (sUndo == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = FormatApi();
        if (wFormatApi == nullptr) {
            return;
        }
        // Most-derived custom IncCheckfo first (not via ClassName — subclasses were missed).
        if (tUndoFormat* wUndoFormat = dynamic_cast<tUndoFormat*>(sUndo)) {
            wUndoFormat->IncCheckfo(wFormatApi);
            return;
        }
        if (tUndoDeleteSheet* wUndo = dynamic_cast<tUndoDeleteSheet*>(sUndo)) {
            wUndo->IncCheckfo(wFormatApi);
            return;
        }
        if (tUndoDeleteRow* wUndo = dynamic_cast<tUndoDeleteRow*>(sUndo)) {
            wUndo->IncCheckfo(wFormatApi);
            return;
        }
        if (tUndoDeleteCol* wUndo = dynamic_cast<tUndoDeleteCol*>(sUndo)) {
            wUndo->IncCheckfo(wFormatApi);
            return;
        }
        if (tUndoInsertRow* wUndo = dynamic_cast<tUndoInsertRow*>(sUndo)) {
            wUndo->IncCheckfo(wFormatApi);
            return;
        }
        if (tUndoInsertCol* wUndo = dynamic_cast<tUndoInsertCol*>(sUndo)) {
            wUndo->IncCheckfo(wFormatApi);
            return;
        }
        // tUndoCellValue, tUndoPaste, tUndoCut, tUndoMove, and other tUndoRaz subclasses.
        if (tUndoRaz* wUndoRaz = dynamic_cast<tUndoRaz*>(sUndo)) {
            wUndoRaz->IncCheckfo(wFormatApi);
        }
    }
  
    /// @brief Check format 3 phase Reset Count  Check
    void tSpreadSheetContainer::CheckFormat() {
        tFormatApi* wFormatApi=FormatApi();
        if (wFormatApi!=nullptr) {
            wFormatApi->ResetCheck();
            
            for(tVectorWorkBook::iterator wIterator=m_VectorWorkBook.begin();wIterator<m_VectorWorkBook.end();wIterator++) {
                m_AllocatorWorkBook(*wIterator)->CheckFormat();
            }
            
            // Get Undo Redo
            tVectorUndo wVectorUndo=tApplication::Instance()->GetUndoVector();
            for(auto wUndo : wVectorUndo) {
                CheckFormatUndo(wUndo);
            }
            tVectorUndo wVectorRedo=tApplication::Instance()->GetRedoVector();
            for(auto wUndo : wVectorRedo) {
                CheckFormatUndo(wUndo);
            }
            wFormatApi->Check();
        }
    }
#endif


#ifdef _DEBUGSK
    tString tSpreadSheetContainer::Debug() {
        tStringStream wStream;
        wStream << "SpreadSheet Container ----------" << endl;
        if (m_ActiveWorkBook!=nullptr)
            wStream << "Active --" << m_ActiveWorkBook->Uri() << endl;
        wStream << "--------------" << endl;
        for (auto wIterator : m_VectorWorkBook) {
            tAllocatorRef wRef=wIterator;
            tWorkBook* wWorkBook = m_AllocatorWorkBook(wRef);
            wStream << wWorkBook->Debug() << endl;
        }
        return(wStream.str());
    }
#endif

    void tSpreadSheetContainer::SetOnCellChange(tOnCellChange* sOnCellChange) {
        m_OnCellChange=sOnCellChange;
    }
                                                    
    void tSpreadSheetContainer::OnCellChange(tCell* sCell,tVariant& sValue) {
        if (m_OnCellChange!=nullptr) {
            m_OnCellChange(sCell,sValue);
        }
    }
    // Json ===================================================================
void tSpreadSheetContainer::JsonBegin() {
    // Set JsonShared Active
    m_JsonSharedString.Begin();
    m_JsonSharedFormula.Begin();
    
    while(!m_ContainerJsonCell.empty()) { m_ContainerJsonCell.pop_back(); }
    m_JsonCachedFormulaValues.clear();
    m_JsonRestoreCachedFormulaValues = false;
}

    void tSpreadSheetContainer::SetJsonRestoreCachedFormulaValues(tBool sValue) {
        m_JsonRestoreCachedFormulaValues = sValue;
    }

    void tSpreadSheetContainer::PushJsonCell(tCell* sCell, const tVariant* sCachedValue) {
        m_ContainerJsonCell.push_back(sCell);
        if (m_JsonRestoreCachedFormulaValues && sCachedValue != nullptr &&
            !sCachedValue->IsExcelNull()) {
            m_JsonCachedFormulaValues[sCell] = *sCachedValue;
        }
#ifdef debugjson
        cout << "Push JsonCell Push(" <<  sCell->tItem::StrRef(true) << ")=" << sCell->FormulaStr() << ":" << sCell->Value() << endl;
#endif
    }

    void tSpreadSheetContainer::JsonEndShared() {
        m_JsonSharedString.End();
        m_JsonSharedFormula.End();
    }

    void tSpreadSheetContainer::CompileQueuedJsonCells(tVectorCell* sVectorCellCalculate,
                                                       tSize* sTotalFormulaCells) {
        tSize wTotalFormulaCells = 0;
        while (!m_ContainerJsonCell.empty()) {
            tCell* wCell = m_ContainerJsonCell.front();
            m_ContainerJsonCell.pop_front();
#ifdef debugjson
            cout << "Compil JsonCell (" << wCell->tItem::StrRef() << "):" << wCell->Value() << endl;
#endif
            if (wCell->Value().IsString()) {
                ++wTotalFormulaCells;
                tString wFormulaStr = wCell->Value().Str();
                tBool wUseCachedValue = false;
                tVariant wCachedValue;
                tTempoRect wSavedAfo;
                tBool wHadAfo = false;
                {
                    tTempoRect wAfo = wCell->ArrayFormulaOutputRect();
                    if (wAfo.IsValid()) {
                        wSavedAfo = wAfo;
                        wHadAfo = true;
                    }
                }
                auto wCachedIt = m_JsonCachedFormulaValues.find(wCell);
                if (wCachedIt != m_JsonCachedFormulaValues.end()) {
                    wCachedValue = wCachedIt->second;
                    wUseCachedValue = !wCachedValue.IsExcelNull();
                    m_JsonCachedFormulaValues.erase(wCachedIt);
                }
                if (!wUseCachedValue) {
                    wCell->Value(tVariant());
                }

                tBool wResult = m_LemonInterface.Compil(m_ActiveWorkBook, wCell, wFormulaStr.c_str());

                if (wResult) {
                    tFormula* wFormula = wCell->Formula();
                    if (wFormula != nullptr) {
                        if (!wFormula->BitSetVolatile().Empty()) {
                            wCell->ColRowCellRange()->AddVolatile(wCell);
                        }
                    }
                    if (wUseCachedValue) {
                        wCell->Value(wCachedValue);
                        if (wHadAfo) {
                            wCell->SetSpillRect(wSavedAfo.Top(), wSavedAfo.Left(), wSavedAfo.Bottom(),
                                                wSavedAfo.Right());
                        }
                    } else {
                        sVectorCellCalculate->push_back(wCell);
                        // OOXML one-row spill refs: keep spillrange when EndCalculate runs without cached v/t.
                        if (wHadAfo) {
                            wCell->SetSpillRect(wSavedAfo.Top(), wSavedAfo.Left(), wSavedAfo.Bottom(),
                                                wSavedAfo.Right());
                        }
                    }
                } else {
                    cerr << wCell->StrRef(true) << "->Error compil:"
                         << LemonInterface()->ErrorWithDetail() << endl;
                    // Keep persisted display value even when compile fails (load must finish).
                    if (wUseCachedValue) {
                        wCell->Value(wCachedValue);
                        if (wHadAfo) {
                            wCell->SetSpillRect(wSavedAfo.Top(), wSavedAfo.Left(), wSavedAfo.Bottom(),
                                                wSavedAfo.Right());
                        }
                    }
                }
            }
        }
        if (sTotalFormulaCells != nullptr) {
            *sTotalFormulaCells = wTotalFormulaCells;
        }
    }

    void tSpreadSheetContainer::JsonEndCellsOnly() {
        JsonEndShared();
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        tVectorCell wVectorCellCalculate;
        CompileQueuedJsonCells(&wVectorCellCalculate);
        for (auto wCellRef : wVectorCellCalculate) {
            wContainerPath.Add(wCellRef);
        }
        wContainerPath.EndCalculate();
        m_JsonCachedFormulaValues.clear();
        m_JsonRestoreCachedFormulaValues = false;
    }

    void tSpreadSheetContainer::JsonEnd() {
        JsonEndShared();
        
        // Compile named formula bodies (formulanamed) onto the named-formula sheet. namedranges / formulanamed JSON
        // are applied in WorkBook::Json before sheet cells. Do not queue named cells on tContainerPath until after
        // the JsonCell compile loop (otherwise Path()!=0 blocks Calculation inside MultiCellSpillRangeForFormulaNamed).
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();

        tVectorCell  wVectorCellCalculate;
        tSize wTotalFormulaCells = 0;

        // Benchmark only (JsonEndProfile): split compile vs calculate wall time.
        const std::clock_t wProfCompilStart = m_JsonEndProfile ? std::clock() : 0;

        // Compîl and make DEPENDANT ===========================================
        m_ActiveWorkBook->RangeNamedContainer()->JsonCompil();
        
 #ifdef debugjson
 #ifdef _DEBUGSK
         cout << m_ActiveWorkBook->RangeNamedContainer()->Debug();
 #endif
 #endif
 
        CompileQueuedJsonCells(&wVectorCellCalculate, &wTotalFormulaCells);
        const std::clock_t wProfCalcStart = m_JsonEndProfile ? std::clock() : 0;
#ifdef checksp
        Check();
#endif
        const tBool wDeferRecalc = m_JsonRestoreCachedFormulaValues;
        const tSize wUncachedCount = wVectorCellCalculate.size();
        // Large workbooks (Ref.sker): a few formulas missing v/t must not trigger full EndCalculate.
        // Calendars / exports with mostly uncached formulas still need targeted recalc (>=10% uncached).
        const tSize wUncachedPct =
            (wTotalFormulaCells > 0) ? ((wUncachedCount * 100) / wTotalFormulaCells) : 0;
        const tBool wNeedsUncachedRecalc =
            wDeferRecalc && wUncachedCount > 0 && wUncachedPct >= 10;
        if (wDeferRecalc) {
            m_ActiveWorkBook->RelinkJsonPersistedSpillSlaves();
        }
        // Defer-recalc: skip EndCalculate when every formula cell had persisted v/t (large workbooks).
        // Spill calendars / SkExcel exports without v/t still need matrix named formulas + uncached cells.
        m_ActiveWorkBook->RangeNamedContainer()->AddFormulaNamedCellsToPath(
            &wContainerPath, wDeferRecalc && !wNeedsUncachedRecalc);
        if (!wDeferRecalc || wNeedsUncachedRecalc) {
            for (auto wCellRef : wVectorCellCalculate) {
                wContainerPath.Add(wCellRef);
            }
        }
        wContainerPath.EndCalculate();
        if (wNeedsUncachedRecalc) {
            for (auto wCellRef : wVectorCellCalculate) {
                if (wCellRef != nullptr && wCellRef->IsSpillRange()) {
                    wCellRef->PruneMatrixSpillOutsideArrayFormulaOutput();
                }
            }
        }
        if (m_JsonEndProfile) {
            const std::clock_t wProfEnd = std::clock();
            std::cout << "[JsonEnd] compile "
                      << ((double)(wProfCalcStart - wProfCompilStart) * 1000.0 / CLOCKS_PER_SEC)
                      << " ms (" << wTotalFormulaCells << " formula cells), calculate "
                      << ((double)(wProfEnd - wProfCalcStart) * 1000.0 / CLOCKS_PER_SEC)
                      << " ms (" << wUncachedCount << " cells)" << std::endl;
        }
        m_JsonCachedFormulaValues.clear();
        m_JsonRestoreCachedFormulaValues = false;
 #ifdef debugjson
 #ifdef _DEBUGSK
        cout << "End Json Load" << endl;
        cout << m_ActiveWorkBook->RangeNamedContainer()->Debug();
 #endif
 #endif
 
    }

    void tSpreadSheetContainer::CurrentJsonCell(tCell* sCell) { m_CurrentJsonCell=sCell; }

    tCell* tSpreadSheetContainer::CurrentJsonCell() { return(m_CurrentJsonCell); }

    tJsonSharedString*  tSpreadSheetContainer::JsonSharedString() { return (&m_JsonSharedString); }
    tJsonSharedString*  tSpreadSheetContainer::JsonSharedFormula() { return(&m_JsonSharedFormula); }
 
    void tSpreadSheetContainer::JsonShared(Writer<StringBuffer>* sWriter) const {
        m_JsonSharedString.Json(sWriter);
        m_JsonSharedFormula.Json(sWriter);
    }

    void tSpreadSheetContainer::JsonShared(const rapidjson::Value& sValue) {
        m_JsonSharedString.Json(sValue);
        m_JsonSharedFormula.Json(sValue);
    }

	tSpreadSheetContainer* tSpreadSheetContainer::Instance(){
		if (wStaticSpreadSheet  == nullptr) {
			wStaticSpreadSheet = new  tSpreadSheetContainer();
		}
		return(wStaticSpreadSheet);
	}

	void DoneSpreadSheet() {
		if (wStaticSpreadSheet != nullptr) {
			delete(wStaticSpreadSheet);
			wStaticSpreadSheet = nullptr;
		}
		tStaticColRowCellRange::Done();
	}


}
