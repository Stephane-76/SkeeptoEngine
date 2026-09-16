//=============================================================================
// SkSpreadSheet Formula
//=============================================================================
#ifndef SkFormula_hpp
#define SkFormula_hpp

#include "SkApplication.hpp"
#include "SkTools.hpp"
#include "SkLexerSpreadSheet.hpp"
#include "SkColRow.hpp"
#include "SkItem.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {
	
   
    enum class tVolatile : tByte { t_None=0, t_All=1,t_Row=2,t_Col=3 };

    // Interface Mode on Api & tUndoSpreadSheet
    // used by tFunction to know if the function is volatile (Sheet, Row, Col) or not
    typedef tBitSet<tChar> tBitSetVolatile;


	//=========================================================================
	//! Item compose an element in formula
	class tItemFormula : public tClass {
	private:
		//! Kind of element
		tKind		   m_Kind;
		//! Value of element (use extra value of variant when use tRange).
		tVariant	   m_Value;
	public:
		/// @brief      Constructor SkFormula.
		tItemFormula();

		/// @brief      Constructor SkFormula with kind and variant.
		/// @param[in]  sKind tKind
		/// @param[in]  sVarianttVariant
		tItemFormula(tKind sKind,const tVariant& sVariant);
		
		/// @brief      Constructor of copy.
		/// @param[in]  sItemFormula const tItemFormula&
		tItemFormula(const tItemFormula& sItemFormula);
		
		/// @brief      Destructor clear m_Value.
		~tItemFormula(); 


		/// @brief      Return kind.
		/// @return		tKind
		tKind Kind();
		
		/// @brief      Return kind (const version).
		/// @return		tKind
		tKind Kind() const;

		/// @brief      Return variant.
		/// @return		SkVariant
		tVariant& Value();
		
		/// @brief      Return variant (const version).
		/// @return		const SkVariant&
		const tVariant& Value() const;
        ///  @brief        ReadJson
        /// @param[in]  sValue const Value&
        void Json(const rapidjson::Value& sValue);

        ///  @brief        WriteJson
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

	};


	typedef vector<tItemFormula> tVectorItemFormula;
	//=========================================================================
	//! Contained in tSharedFormula (gain of memory)
	class tFormula : public tClass {
	private:
		//! Formula Key  (Keep Only one formula for the same strcuctture).
		tString             m_FormulaKey;
		//! Vector of item formula (make by lemon).
		tVectorItemFormula  m_VectorItemFormula;
		//! Correspondence push order -> lex order: Order(i) = IndLex of ref at push index i.
		tVectorInt          m_VectorOrderLemon;

        //! Forrrecalculate
        tBitSetVolatile m_BitSetVolatile;
        
        /// @brief      Return Ref of cell or Range or RangeNamed.
        /// @param[in]  sCellRoot tCell*
        /// @param[in]  sLex SkLexerSpreadSheet*
        /// @param[in]  sIndex tIindex
        /// @param[in]  sToken SkLexerToken*
        /// @param[in]  sIsRangeNamed tBool
        /// @param[in]  sR1C1 tBool
        /// @return     tString
        /// @param[out] sExtraRefs number of extra VectorRef entries consumed (non-zero
        ///             only for a multi-area named range group). The caller must add
        ///             this to its local index to skip the sibling areas.
        SkInline tString ClassOrRangeStr(const tCell* sCellRoot, tLexer* sLex, tIndex sIndex, tLexerToken* sToken,tBool& sIsCellClassOrRangeNamed, tIndex& sExtraRefs, tBool sR1C1=false) const;
        
        ///@brief      Get table string
        /// @param[in]  sCellRoot const tCell*
        /// @param[in]  sLex tLexer*
        /// @param[in]  sIndex tIndex
        /// @param[in]  sToken tLexerToken*
        /// @param[in]  sSep tString
        /// @param[in]  sArg tChar
        /// @param[in]  sUser tBool
        /// @param[in]  sExplicitTableName optional lexer lexeme for table id (e.g. PaymentSchedule3) when geometry-based lookup fails
        /// @return     tString
        SkInline tString TableStr(const tCell* sCellRoot, tLexer* sLex, tIndex sIndex,tLexerToken* sToken,tString sSep,tChar sArg, tBool sUser=false, const tString* sExplicitTableName=nullptr) const;
	public:
		/// @brief      Constructor.
		tFormula();
		
		/// @brief      Constructor of copy.
		/// @param[in]  sFormula tFormula&
		tFormula(const tFormula& sFormula);

		/// @brief      Destructor call clear.
		~tFormula();

		
		/// @brief      clear vector of item.
		void Clear();

		/// @brief      Return pointer of m_VectorItemFormula.
		/// @return		tVectorItemFormula
		tVectorItemFormula* VectorItemFormula();

		/// @brief      Return pointer of m_VectorItemFormula (const).
		/// @return		const tVectorItemFormula*
		const tVectorItemFormula* VectorItemFormula() const;

		/// @brief      Return formula key.
		/// @return		tString
		tString FormulaKey() const;
		
		/// @brief      Set formula key.
		/// @param[in]	sFormulaKey tString
		void FormulaKey(tString sFormulaKey);

		/// @brief      Return key (for operator SharedFormula)
		/// @return		tString const 
		tString Key() const;
        
        /// @brief      Rebuild display string from FormulaKey (re-lex with US decimal, then apply locale).
        ///             Commas map to tLocale::Arg() outside { } and | |; inside { } / | | they stay column separators.
        ///             Float lexemes use locale decimal (e.g. . -> comma in FR). If FormulaKey holds a float token
        ///             (e.g. merged "1,1" at compile time under FR), display shows one localized number, not two ints.
        /// @param[in]  sCellRoot const tCell*
        /// @param[in]  sR1C1 tBool
        /// @param[in]  sUser (Table not [#This Row]
        /// @return tString
        tString Str(const tCell* sCellRoot, tBool sR1C1=false,tBool sUeer=false) const;

		/// @brief      Set item formula (call by lemon).
		/// @param[in]	sKind tKind
		/// @param[in]	sVariant tVariant
		void Push(tKind sKind, tVariant& sVariant);
		/*
		/// @brief      Record lex order of ref at current push index (correspondence push order -> lex).
		tInt  Ref(tInt sPushIndex) const;

    
        tBool PushRefOrder(tKind sKind, tItem* sItem,tVectorItem* sVectorItem);

        tBool KeepOrder();
        */
      
        /// @brief      Set volatile
        /// @param[in]  sVolatile  sVolatile
        void SetVolatile(tVolatile sVolatile);
        
        /// @brief      Gett volatile
        /// @param[in]  sVolatile  sVolatile
        /// @return tBool
        tBool Volatile(tVolatile sVolatile) const;
        
        /// @brief      Get volatile
        /// @return tVolatile
        tBitSetVolatile BitSetVolatile();
        
        /// @brief      Get volatile (const version)
        /// @return tVolatile
        tBitSetVolatile BitSetVolatile() const;
        
        /// @brief      Return if row is volatile.
        /// @return        tBool
        tBool IsVolatile();
        
        /// @brief      Return if row is volatile (const version).
        /// @return        tBool
        tBool IsVolatile() const;

		/// @brief      Return if row is volatile.
        /// @return        tBool
		tBool IsRowVolatile();
		
		/// @brief      Return if row is volatile (const version).
        /// @return        tBool
		tBool IsRowVolatile() const;


		/// @brief      Return if col is volatile.
        /// @return        tBool
		tBool IsColVolatile();
		
		/// @brief      Return if col is volatile (const version).
        /// @return        tBool
		tBool IsColVolatile() const;
        
        ///  @brief        ReadJson
        /// @param[in]  sValue const Value&
        void Json(const Value& sValue);

        ///  @brief        WriteJson
        /// @param[in]  sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

		/// @brief      Operator() return formula key.
		/// @return		tString
		tString operator()() const;

		/// @brief      Debug.
        /// @return tString
		tString Debug() const;
#ifdef checksp
		/// @brief      Check.
		void Check() const;
#endif

	};


} // End of namespace

#endif
