//=============================================================================
// SkJavascriptFunction.hpp
// Api for React wasm
//=============================================================================
#ifndef SkJavascriptFunction_hpp
#define SkJavascriptFunction_hpp


#include <SkApplication.hpp>
#include <SkSpreadSheet.hpp>
#include <SkApi.hpp>

// After because std::function conflict 
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;
#endif

using namespace SkRoot;

namespace SkSpreadSheet {
    class tFunctionJavascript;
    //=========================================================================
    //! Call back for function javascript
    class tCallBackRangeJavascript : public tCallBackRangeFunction {
    private:
        Writer<StringBuffer>*   m_Writer;
        tFunctionJavascript*    m_Parent;
    public:
        /// @brief        Constructor SkCallBackRange with owner sColRowCellRange.
        /// @param[in]  sColRowCellRange tColRowCellRange*
        tCallBackRangeJavascript(tColRowCellRange* sColRowCellRange, Writer<StringBuffer>* sWriter,tFunctionJavascript*    sParent);
        
        /// @brief        Call method for calculate m_Value if return false stop process.
        /// @param[in]    sAllocatorRef tInt Index on allocator cell
        virtual tBool CallBack(tAllocatorRef sAllocatorRef); // Return false for Stop
    };

    //=========================================================================
    //! Function Javascript
    class tFunctionJavascript : public tFunction {
    private:
        tString m_Name;
        tBool   m_Ref;
    public:
        /// @brief        Constructor SkFunctionSum.
        tFunctionJavascript(tString sName,tInt sNbArg,tBool sRef=true);

        tString CallJavascript(tString sArg);

        /// @brief        Pop stack args and delegate to CallWithArgs.
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        
        /// @brief        Call JavaScript function with extracted arguments.
        /// @param[in]  sArgVector std::vector<tStackElem>* vector of arguments
        tVariant CallWithArgs(std::vector<tStackElem>* sArgVector);
    };


}; // End of namespace




#endif
