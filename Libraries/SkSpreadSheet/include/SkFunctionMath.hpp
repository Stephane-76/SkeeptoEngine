//=============================================================================
// SkSpreadSheet Function Math 
//=============================================================================
#ifndef SkFunctionMath_hpp
#define SkFunctionMath_hpp

#include "SkFunction.hpp"

namespace SkSpreadSheet {

	// SpillKind: classes that override SpillKind() in .cpp return Aggregate (SUM, MIN, MAX, PRODUCT, SUMPRODUCT, RAND).
	// All other math functions (SIN, COS, ABS, ROUND, MOD, POWER, ...) inherit tFunction::SpillKind(),
	// which defaults to ElementWise — same idea as Excel 365 spilling unary math over a range.

	//=========================================================================
	//! Call back for function average (sum + count of numeric cells only)
	class tCallBackRangeAverage : public tCallBackRangeFunction {
	private:
		tInt m_Count;
	public:
		tCallBackRangeAverage(tColRowCellRange* sColRowCellRange);

		virtual tBool CallBack(tAllocatorRef sAllocatorRef);

		tInt Count() const { return m_Count; }
	};

	//=========================================================================
	//! Call back for function sum
	class tCallBackRangeSum : public tCallBackRangeFunction {
	public:
		/// @brief		Constructor SkCallBackRangeSum with owner sColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackRangeSum(tColRowCellRange* sColRowCellRange);
		
		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tInt Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef); // Return false for Stop
	};

	//=========================================================================
	//! Function sum
	class tFunctionSum : public tFunction {
	public:
		/// @brief		Constructor SkFunctionSum.
		tFunctionSum();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

    //=========================================================================
    //! Function Average
    class tFunctionAverage : public tFunction {
    public:
        /// @brief        Constructor SkFunctionSum.
        tFunctionAverage();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

	//=========================================================================
	//! AVERAGEA — average with text→0 / TRUE→1 / FALSE→0 (range rules).
	class tFunctionAverageA : public tFunction {
	public:
		tFunctionAverageA();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Median (Excel MEDIAN) — middle value of the numeric arguments / ranges.
	class tFunctionMedian : public tFunction {
	public:
		tFunctionMedian();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! STDEV_S / STDEV_P / VAR_S / VAR_P / STDEVA / STDEVPA / VARA / VARPA.
	//! Dotted Excel names (STDEV.S, …) are rewritten to undotted engine names on import.
	enum class tVarianceKind : tByte {
		StdevS = 0,  // sample stdev (n−1), sqrt
		StdevP = 1,  // population stdev (n), sqrt
		VarS = 2,    // sample variance (n−1)
		VarP = 3,    // population variance (n)
		StdevA = 4,  // sample stdev, *A coercion
		StdevPA = 5, // population stdev, *A coercion
		VarA = 6,    // sample variance, *A coercion
		VarPA = 7    // population variance, *A coercion
	};

	class tFunctionVariance : public tFunction {
	private:
		tVarianceKind m_Kind;
	public:
		explicit tFunctionVariance(tVarianceKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SKEW / SKEW_P — sample or population skewness.
	class tFunctionSkew : public tFunction {
	private:
		tBool m_Population;
	public:
		explicit tFunctionSkew(tBool sPopulation);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! KURT — excess kurtosis of a data set.
	class tFunctionKurt : public tFunction {
	public:
		tFunctionKurt();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! STANDARDIZE(x, mean, standard_dev) — (x − mean) / standard_dev.
	class tFunctionStandardize : public tFunction {
	public:
		tFunctionStandardize();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! FISHER / FISHERINV — Fisher transformation and its inverse.
	class tFunctionFisher : public tFunction {
	private:
		tBool m_Inverse;
	public:
		explicit tFunctionFisher(tBool sInverse);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! PHI — standard normal probability density φ(x).
	class tFunctionPhi : public tFunction {
	public:
		tFunctionPhi();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! GAUSS(x) — NORMSDIST(x) − 0.5.
	class tFunctionGauss : public tFunction {
	public:
		tFunctionGauss();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! NORM.S.DIST / NORMSDIST — standard normal PDF or CDF.
	//! Legacy NORMSDIST takes only z and always returns the CDF.
	class tFunctionNormSDist : public tFunction {
	private:
		tBool m_Legacy;
	public:
		explicit tFunctionNormSDist(tBool sLegacy);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! NORM.DIST / NORMDIST — normal PDF or CDF for mean / standard_dev.
	class tFunctionNormDist : public tFunction {
	public:
		tFunctionNormDist();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! NORM.S.INV / NORMSINV / NORM.INV / NORMINV — inverse normal CDF.
	//! Standard form takes probability only (mean 0, sd 1).
	class tFunctionNormInv : public tFunction {
	private:
		tBool m_Standard;
	public:
		explicit tFunctionNormInv(tBool sStandard);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! GAMMA / GAMMALN / GAMMALN_PRECISE — Γ(x) or ln(Γ(x)).
	class tFunctionGamma : public tFunction {
	private:
		tBool m_Ln;
	public:
		explicit tFunctionGamma(tBool sLn);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! AVEDEV — average of absolute deviations from the mean.
	class tFunctionAveDev : public tFunction {
	public:
		tFunctionAveDev();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! DEVSQ — sum of squared deviations from the mean.
	class tFunctionDevSq : public tFunction {
	public:
		tFunctionDevSq();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! CORREL / PEARSON — Pearson correlation of two equal-sized arrays.
	class tFunctionCorrel : public tFunction {
	public:
		tFunctionCorrel();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! COVARIANCE_P / COVARIANCE_S — population (n) or sample (n−1) covariance.
	class tFunctionCovariance : public tFunction {
	private:
		tBool m_Sample;
	public:
		explicit tFunctionCovariance(tBool sSample);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! PERCENTILE_INC / PERCENTILE_EXC / PERCENTILE — k-th percentile (k in 0..1).
	class tFunctionPercentile : public tFunction {
	private:
		tBool m_Exclusive;
	public:
		explicit tFunctionPercentile(tBool sExclusive);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! QUARTILE_INC / QUARTILE_EXC / QUARTILE — quartile via percentile family.
	class tFunctionQuartile : public tFunction {
	private:
		tBool m_Exclusive;
	public:
		explicit tFunctionQuartile(tBool sExclusive);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SLOPE / INTERCEPT / RSQ / STEYX — linear regression of known_y on known_x.
	enum class tLinRegKind : tByte { Slope = 0, Intercept = 1, Rsq = 2, Steyx = 3 };

	class tFunctionLinReg : public tFunction {
	private:
		tLinRegKind m_Kind;
	public:
		explicit tFunctionLinReg(tLinRegKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! FORECAST / FORECAST_LINEAR(x, known_y, known_x) — predict y at x.
	class tFunctionForecast : public tFunction {
	public:
		tFunctionForecast();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! TREND(known_y's, [known_x's], [new_x's], [const]) — predicted y along a linear fit (spills).
	//! Single independent variable only (known_x same shape as known_y, or default {1..n}).
	class tFunctionTrend : public tFunction {
	public:
		tFunctionTrend();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! GROWTH(known_y's, [known_x's], [new_x's], [const]) — exponential fit predictions (spills).
	//! Fits ln(y) = ln(b) + ln(m)·x (or b=1 when const is FALSE). known_y must be > 0.
	class tFunctionGrowth : public tFunction {
	public:
		tFunctionGrowth();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! LINEST(known_y's, [known_x's], [const], [stats]) — linear regression coefficients (spills).
	//! Single independent variable. stats TRUE → 5×2 stats array; FALSE/omitted → 1×2 (m, b).
	class tFunctionLinEst : public tFunction {
	public:
		tFunctionLinEst();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! PERCENTRANK_INC / PERCENTRANK_EXC — percentage rank of x in a data set.
	class tFunctionPercentRank : public tFunction {
	private:
		tBool m_Exclusive;
	public:
		explicit tFunctionPercentRank(tBool sExclusive);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! PERMUT / PERMUTATIONA — P(n,k) or n^k (with repetition).
	class tFunctionPermut : public tFunction {
	private:
		tBool m_WithRep;
	public:
		explicit tFunctionPermut(tBool sWithRep);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! GEOMEAN — geometric mean (all values must be > 0).
	class tFunctionGeoMean : public tFunction {
	public:
		tFunctionGeoMean();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! HARMEAN — harmonic mean (no zeros).
	class tFunctionHarMean : public tFunction {
	public:
		tFunctionHarMean();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! TRIMMEAN(array, percent) — mean after trimming percent from both ends.
	class tFunctionTrimMean : public tFunction {
	public:
		tFunctionTrimMean();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! FREQUENCY(data_array, bins_array) — vertical count array (spills; bins+1 rows).
	class tFunctionFrequency : public tFunction {
	public:
		tFunctionFrequency();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! LARGE / SMALL — k-th largest (sLarge=true) or smallest (sLarge=false) numeric value.
	class tFunctionNth : public tFunction {
	private:
		tBool m_Large;
	public:
		explicit tFunctionNth(tBool sLarge);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! MODE_SNGL / MODE / MODE_MULT — most frequent numeric value(s).
	//! No duplicates → #N/A. MODE/MODE_SNGL: first mode; MODE_MULT: vertical spill of all modes.
	class tFunctionMode : public tFunction {
	private:
		tBool m_Multi;
	public:
		explicit tFunctionMode(tBool sMulti = false);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! RANK_EQ / RANK / RANK_AVG — rank of a number in a list.
	//! order omitted/0 = descending (largest = 1); nonzero = ascending.
	//! EQ: ties share the top rank. AVG: ties get the average of their ranks.
	class tFunctionRankEq : public tFunction {
	private:
		tBool m_Average;
	public:
		explicit tFunctionRankEq(tBool sAverage = false);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! MAXA / MINA — max/min with text→0 / TRUE→1 / FALSE→0 (range rules).
	class tFunctionMinMaxA : public tFunction {
	private:
		tBool m_Max;
	public:
		explicit tFunctionMinMaxA(tBool sMax);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! ATAN2 / QUOTIENT — shared 2-arg numeric math.
	enum class tBinaryMathKind : tByte {
		Atan2 = 0,
		Quotient = 1
	};

	class tFunctionBinaryMath : public tFunction {
	private:
		tBinaryMathKind m_Kind;
	public:
		explicit tFunctionBinaryMath(tBinaryMathKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! FACT / FACTDOUBLE — factorial or double factorial (truncated toward zero).
	//! Constructor: sDouble=false → FACT; sDouble=true → FACTDOUBLE.
	class tFunctionFact : public tFunction {
	private:
		tBool m_Double;
	public:
		explicit tFunctionFact(tBool sDouble = false);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! EVEN / ODD — round away from zero to the nearest even/odd integer.
	//! Constructor: sEven=true → EVEN; sEven=false → ODD.
	class tFunctionEvenOdd : public tFunction {
	private:
		tBool m_Even;
	public:
		explicit tFunctionEvenOdd(tBool sEven);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! COMBIN / COMBINA — C(n,k) or combinations with repetition C(n+k-1, k).
	//! Constructor: sWithRep=false → COMBIN; sWithRep=true → COMBINA.
	class tFunctionCombin : public tFunction {
	private:
		tBool m_WithRep;
	public:
		explicit tFunctionCombin(tBool sWithRep = false);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! BASE(number, radix, [min_length]) — decimal → text in given base (2..36).
	class tFunctionBase : public tFunction {
	public:
		tFunctionBase();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! DECIMAL(text, radix) — text in given base (2..36) → decimal number.
	class tFunctionDecimal : public tFunction {
	public:
		tFunctionDecimal();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! BITAND / BITOR / BITXOR / BITLSHIFT / BITRSHIFT — bitwise ops on [0, 2^48).
	enum class tBitOpKind : tByte { And = 0, Or, Xor, LShift, RShift };

	class tFunctionBitOp : public tFunction {
	private:
		tBitOpKind m_Kind;
	public:
		explicit tFunctionBitOp(tBitOpKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! DELTA / GESTEP — Kronecker delta / unit step.
	enum class tEngStepKind : tByte { Delta = 0, GeStep };

	class tFunctionEngStep : public tFunction {
	private:
		tEngStepKind m_Kind;
	public:
		explicit tFunctionEngStep(tEngStepKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! BIN2* / DEC2* / HEX2* / OCT2* — Excel engineering radix conversions
	//! (10-digit two's-complement widths: bin 10 / oct 30 / hex 40 bits).
	class tFunctionEngBaseConvert : public tFunction {
	private:
		tInt m_FromRadix;
		tInt m_ToRadix;
	public:
		tFunctionEngBaseConvert(tInt sFromRadix, tInt sToRadix);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! ROMAN(number, [form]) — Arabic → Roman numeral text (form 0..4).
	class tFunctionRoman : public tFunction {
	public:
		tFunctionRoman();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! ARABIC(text) — Roman numeral text → Arabic number.
	class tFunctionArabic : public tFunction {
	public:
		tFunctionArabic();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! MULTINOMIAL(number1, [number2], ...) — (Σni)! / (n1!·n2!·…).
	class tFunctionMultinomial : public tFunction {
	public:
		tFunctionMultinomial();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SERIESSUM(x, n, m, coefficients) — Σ ai · x^(n + (i−1)·m).
	class tFunctionSeriesSum : public tFunction {
	public:
		tFunctionSeriesSum();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! PERCENTOF(data_subset, data_all) — SUM(subset) / SUM(all).
	class tFunctionPercentOf : public tFunction {
	public:
		tFunctionPercentOf();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SUMSQ — sum of squares of numeric arguments / ranges.
	class tFunctionSumSq : public tFunction {
	public:
		tFunctionSumSq();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SUMX2MY2 / SUMX2PY2 / SUMXMY2 — pairwise sum over two equal-sized arrays.
	enum class tSumXKind : tByte {
		X2MinusY2 = 0, // SUMX2MY2: Σ(x² − y²)
		X2PlusY2 = 1,  // SUMX2PY2: Σ(x² + y²)
		XMinusY2 = 2   // SUMXMY2:  Σ((x − y)²)
	};

	class tFunctionSumX : public tFunction {
	private:
		tSumXKind m_Kind;
	public:
		explicit tFunctionSumX(tSumXKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Call back for function min
	class tCallBackRangeMin : public tCallBackRangeFunction {
	private:
		tBool m_FirstCall;
	public:
		/// @brief		Constructor SkCallBackRangeSum with owner sColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackRangeMin(tColRowCellRange* sColRowCellRange);

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tInt Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef); 
	};

	//=========================================================================
	//! Function min
	class tFunctionMin : public tFunction {
	private:
	public:
		/// @brief		Constructor SkFunctionMin.
		tFunctionMin();

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sArgVector  tStackElemVector* vector of arguments
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Call back for function max
	class tCallBackRangeMax : public tCallBackRangeFunction {
	private:
		tBool m_FirstCall;
	public:
		/// @brief		Constructor SkCallBackRangeSum with owner sColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackRangeMax(tColRowCellRange* sColRowCellRange);

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tInt Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef); // Stop
	};

	//=========================================================================
	//! Function max
	class tFunctionMax : public tFunction {
	private:
	public:
		/// @brief		Constructor SkFunctionMax.
		tFunctionMax();

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Round
	class tFunctionRound : public tFunction {
	public:
		/// @brief		Constructor SkFunctionRound.
		tFunctionRound();

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function Trunc
	class tFunctionTrunc : public tFunction {
	public:
		/// @brief		Constructor SkFunctionTrunc.
		tFunctionTrunc();

		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function RoundDown (Excel ROUNDDOWN)
	class tFunctionRoundDown : public tFunction {
	public:
		tFunctionRoundDown();
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function RoundUp (Excel ROUNDUP)
	class tFunctionRoundUp : public tFunction {
	public:
		tFunctionRoundUp();
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! CEILING.MATH / FLOOR.MATH / PRECISE / ISO.CEILING / legacy CEILING / FLOOR.
	//! MATH: (number, [significance=1], [mode=0]); uses |significance|.
	//! PRECISE / ISO.CEILING: (number, [significance=1]); always toward ±∞; |significance|.
	//! Legacy CEILING/FLOOR: (number, significance) required; same-sign significance.
	//=========================================================================
	enum class tCeilingFloorKind : tByte {
		CeilingMath = 0,
		FloorMath,
		CeilingLegacy,
		FloorLegacy,
		CeilingPrecise, // CEILING.PRECISE / ISO.CEILING — always toward +∞
		FloorPrecise    // FLOOR.PRECISE — always toward −∞
	};

	class tFunctionCeilingFloorMath : public tFunction {
	private:
		tCeilingFloorKind m_Kind;
	public:
		explicit tFunctionCeilingFloorMath(tCeilingFloorKind sKind);
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! MROUND(number, multiple) — round to the nearest multiple (half away from zero).
	//! Number and multiple must have the same sign (else #NUM!).
	//=========================================================================
	class tFunctionMRound : public tFunction {
	public:
		tFunctionMRound();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! GCD / LCM — shared (sLcm=false → GCD, true → LCM). Truncates toward zero; uses |n|.
	//=========================================================================
	class tFunctionGcdLcm : public tFunction {
	private:
		tBool m_Lcm;
	public:
		explicit tFunctionGcdLcm(tBool sLcm);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};
	
	//=========================================================================
	//! Function Int (Excel INT): rounds a number down to the nearest integer
	//! (toward negative infinity, so INT(-4.3) = -5).
	class tFunctionInt : public tFunction {
	public:
		tFunctionInt();
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

    //=========================================================================
    //! Function Abs
    class tFunctionAbs : public tFunction {
    private:
    public:
        /// @brief        Constructor SkFunctionAbs.
        tFunctionAbs();

        /// @brief        Call method for calculate m_Value if return false stop process.
        /// @param[in]    sArgVectort tStackElemVector* vector of arguments
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };
	//=========================================================================
	//! Shared unary math (1 numeric argument). Constructor selects the operation.
	enum class tUnaryMathKind : tByte {
		Cos = 0,
		Sin,
		Tan,
		Acos,
		Asin,
		Atan,
		Sqrt,
		Log,      // natural log (current sker LOG semantics; 1-arg)
		Log10,
		Exp,
		Ln,
		Radians,
		Degrees,
		// Hyperbolic / reciprocal trig / extras
		Cosh,
		Sinh,
		Tanh,
		Acosh,
		Asinh,
		Atanh,
		Cot,
		Coth,
		Csc,
		Csch,
		Sec,
		Sech,
		Acot,
		Acoth,
		Sign,
		SqrtPi
	};

	class tFunctionUnaryMath : public tFunction {
	private:
		tUnaryMathKind m_Kind;
	public:
		explicit tFunctionUnaryMath(tUnaryMathKind sKind);
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function Pi
	class tFunctionPi : public tFunction {
	public:
		/// @brief        Constructor SkFunctionPi.
		tFunctionPi();

		/// @brief        Call method for calculate m_Value if return false stop process.
		/// @param[in]    sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function Mod
	class tFunctionMod : public tFunction {
	public:
		/// @brief        Constructor SkFunctionMod.
		tFunctionMod();

		/// @brief        Call method for calculate m_Value if return false stop process.
		/// @param[in]    sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Function Power
	class tFunctionPower : public tFunction {
	public:
		/// @brief        Constructor SkFunctionPower.
		tFunctionPower();

		/// @brief        Call method for calculate m_Value if return false stop process.
		/// @param[in]    sArgVectort tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

	//=========================================================================
	//! Call back for function product
	class tCallBackRangeProduct : public tCallBackRangeFunction {
	public:
		/// @brief		Constructor SkCallBackRangeProduct with owner sColRowCellRange.
		/// @param[in]  sColRowCellRange tColRowCellRange*
		tCallBackRangeProduct(tColRowCellRange* sColRowCellRange);
		
		/// @brief		Call method for calculate m_Value if return false stop process.
		/// @param[in]	sAllocatorRef tInt Index on allocator cell
		virtual tBool CallBack(tAllocatorRef sAllocatorRef); // Return false for Stop
	};

	//=========================================================================
	//! Function Product
	class tFunctionProduct : public tFunction {
	public:
		/// @brief		Constructor SkFunctionProduct.
		tFunctionProduct();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function SumProduct (Excel SUMPRODUCT)
	class tFunctionSumProduct : public tFunction {
	public:
		/// @brief		Constructor SkFunctionSumProduct.
		tFunctionSumProduct();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Rand
	class tFunctionRand : public tFunction {
	public:
		/// @brief		Constructor SkFunctionRand.
		tFunctionRand();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function RandBetween
	class tFunctionRandBetween : public tFunction {
	public:
		/// @brief		Constructor SkFunctionRandBetween.
		tFunctionRandBetween();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
	};

}; // End of namespace

#endif
