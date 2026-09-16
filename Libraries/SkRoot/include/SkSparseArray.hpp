//================================================e=============================
// Skeema SkSparseArray
/**
* @page SkSparseArray
* @par
* @par Use for gain of memory of large array...
*/
//=============================================================================
#ifndef SkSparseArray_hpp
#define SkSparseArray_hpp

#include <vector>
#include "../include/SkTypes.hpp"
#include "../include/SkClass.hpp"

#define _sparsearraydebug

namespace SkRoot {
    //=========================================================================
    //! Class use for CallBack 
    template <class T>
    class tSparseArrayCallBack : public tClass {
    public:
        /// @brief		Constructor SkSparseArrayCallBack.
        tSparseArrayCallBack() : tClass() {};

        /// @brief		Destructor SkSparseArrayCallBack.
        virtual ~tSparseArrayCallBack() {};

        /// @brief		Method Call by sparse array  (Callback)
        /// @param[in]  sValue T Element
        /// @return		tBool if false stop processus
        virtual tBool CallBack(T /*sValue*/) { return(false); } // Stop
    };

    //=========================================================================
    //! Template class use for "SparseArray", not pointer for T
    template <class T>
    class tSparseArray : public tClass {
    protected:
        tSize m_SizeTrack;
        //! Vector of element
        typedef vector<vector<T>*>  tVectorTrack;
        //! Empty Value
        T	m_EmptyValue;
        //! Vector of element
        tVectorTrack m_VectorTrack;
    public:
        /// @brief      Constructor SkSparseArray by default Size track = 512.
        tSparseArray() : tClass(), m_EmptyValue(){
            m_SizeTrack = 256;
        }
        /// @brief      Constructor SkSparseArray with size of track.
        /// @param[in]  sSizeTrack tSize size of track
        tSparseArray(tSize sSizeTrack) : tClass(), m_EmptyValue(){
            m_SizeTrack = sSizeTrack;
        }

        /// @brief      Destructor SkSparseArray.
        virtual ~tSparseArray() {
            Clear();
        }

    
        /// @brief      Method Clear if if erase = true delete Item
        /// @param[in]  sDelete  tBool default = true
        virtual void Clear(tBool sDelete=true) {
            if (sDelete) {
                for (auto wElem : m_VectorTrack) {
                    delete(wElem);
                }
            }
            m_VectorTrack.clear();
        }

        /// @brief      Get by global index the index of track and indice on track.
        /// @param[in]  sIndex tSize Value de 0..N of the element
        /// @return     tuple(Index,Indice)
        SkInline tuple<tSize, tSize> ReturnTrackIndex(tSize sIndex) {
#ifdef sparsearraydebug
            cout << " SkSparse tuple(" << sIndex <<  ")";
            cout << "     -->" << (sIndex / m_SizeTrack) << ":" << sIndex % m_SizeTrack << endl;
#endif
            return(make_tuple(sIndex / m_SizeTrack, sIndex % m_SizeTrack));
        }

        /// @brief      Get global index by index track and indice on track.
        /// @param[in]  sTrack index of track
        /// @param[in]  sIndexTrack index in track
        /// @return     S Global index 
        SkInline tSize ReturnIndice(tSize sTrack, tSize sIndexTrack) {
            return(m_SizeTrack * sTrack + sIndexTrack);
        }
        
    
        /// @brief      Method Get element for reading.
        /// @param[in]  sIndex tSize Index Global 
        /// @return     T& Refrence on element
        T& operator()(tSize sIndex) {
            tSize wTrackIndex;
            tSize wIndex;
#ifdef sparsearraydebug
            cout << "operator(" << sIndex << ":"  << m_SizeTrack << " * "  << m_VectorTrack.size() << ")->";
#endif
            tie(wTrackIndex, wIndex) = ReturnTrackIndex(sIndex);
           
            if (wTrackIndex >= m_VectorTrack.size()) {
#ifdef sparsearraydebug
                cout << "wTrackIndex >= m_VectorTrack.size()" << endl;
#endif
                return(m_EmptyValue);

            }
            
            vector<T>* wTrack = m_VectorTrack.at(wTrackIndex);
            if (wTrack == nullptr) {
#ifdef sparsearraydebug
                cout << "wTrack==nullptr" << endl;
#endif
                return(m_EmptyValue);
            }
             
            if (wIndex<wTrack->size()) return(wTrack->at(wIndex));
#ifdef sparsearraydebug
            cout << "wTrack==nullptr" << endl;
#endif
            return(m_EmptyValue);
        }

        /// @brief      Method Get element for writing.
        /// @param[in]  sIndex tSize Index Global 
        /// @return     T& Reference on element
        SkInline T& operator[](tSize sIndex)  {
            tSize wTrackIndex;
            tSize wIndex;
            tie(wTrackIndex, wIndex) = ReturnTrackIndex(sIndex);
            // Enlarges vector
            if (wTrackIndex >= m_VectorTrack.size()) {
                m_VectorTrack.resize(wTrackIndex + 1); // Enlarge one by ten 
            }
            // is Alloc new Track
            vector<T>* wTrack = m_VectorTrack.at(wTrackIndex);
            if (wTrack == nullptr) {
                wTrack = new vector<T>;
                m_VectorTrack.at(wTrackIndex) = wTrack;
            }
            // Necessary Track size change
            if (wIndex>=wTrack->size()) wTrack->resize(wIndex+10);
                
            return(wTrack->at(wIndex));
        }

        /// @brief      Method Get global size of sparse array.
        /// @return     tSize 
        tSize Size() {
            return(m_VectorTrack.size() * m_SizeTrack);
        }
        
        /// @brief      Delete element in sparse array.
        /// @param[in]  sIndex tSize Position
        void Delete(tSize sIndex) {
            tSize wTrackIndex;
            tSize wIndex;
#ifdef sparsearraydebug
            cout << "SkSparse Delete(" << sIndex <<  ")" << endl;
#endif
            tie(wTrackIndex, wIndex) = ReturnTrackIndex(sIndex);
        
            if (wTrackIndex >= m_VectorTrack.size()) {
                return;
            }
            
            vector<T>* wTrack = m_VectorTrack.at(wTrackIndex);
            if (wTrack == nullptr) {
                return;
            }
             
            if (wIndex<wTrack->size()) {
                delete(wTrack->at(wIndex));
                wTrack->at(wIndex)=0;
            }
        }

        /// @brief      Method Get size Real.
        /// @return     tSize 
        tSize RealSize() {
            tSize wGlobal = 0;  
            if (m_VectorTrack.size() > 0) {
                // Get total value with track
                wGlobal=m_VectorTrack.size() * m_SizeTrack;
                vector<T>* wTrack = m_VectorTrack.at(m_VectorTrack.size() - 1);
#ifdef sparsearraydebug
                cout << "RealSize =" << m_VectorTrack.size()  << " * " << m_SizeTrack << "=" << wGlobal << " ->";
#endif
                tSize wIndex = wTrack->size() - 1;
                while (((tSize)(*wTrack)[wIndex] == 0) && (wIndex>0)) {
                    wIndex--;
                }
                wGlobal-=(m_SizeTrack-wIndex);
            }
            //cout << "wGlobal After=" << wGlobal << endl;
#ifdef sparsearraydebug
            cout << "SkSparse RealSize=" << wGlobal <<  endl;
#endif
            return(wGlobal);
        }

        /// @brief      insert element in sparse array.
        /// @param[in]  sIndex tSize Position
        /// @param[in]  sSize tSize length
        SkInline void Insert(tSize sIndex, tSize sSize) {
            tSize wEnd=Size()-1+sSize;
            T wItem;

            // Move Element ==================================================
            for (tSize wIndex = wEnd; wIndex >= sIndex + sSize; wIndex--) {
                wItem = (*this)[wIndex - sSize];
                (*this)[wIndex]=wItem;
            }
            // Set empty Value for new insert line ===========================
            for (tSize wIndex = sIndex; wIndex < sIndex + sSize; wIndex++) {
                (*this)[wIndex] = T{};
            }
        }

        /// @brief      Callback while delete class.
        /// @param[in]  sIndex tSize Position
        /// @param[in]  sSize tSize length
        virtual void DeleteClass(tSize /*sIndex*/, tSize /*sSize*/) {}

        /// @brief      erase element in sparse array.
        /// @param[in]  sIndex Position
        /// @param[in]  sSize length
        void Erase(tSize sIndex, tSize sSize) {
#ifdef sparsearraydebug
            cout << "SkSparse Erase(" << sIndex << "," << sSize <<  ")" << endl;
#endif
            // Important for Pt
            DeleteClass(sIndex, sSize);
            // Move all value
            tSize wEnd = Size();
            for (tSize wIndex = sIndex; wIndex < wEnd; wIndex++) {
                T wItem =(*this)[wIndex + sSize];
                // Move Last on current
                (*this)[wIndex] = wItem;
                // Place empty Value
                (*this)[wIndex + sSize] = T{};
            }
            CleanLastTrackAfterErase();
        }

        /// @brief      erase bottom empty elements in sparse array.
        void CleanLastTrackAfterErase() {
#ifdef sparsearraydebug
            cout << "CleanLastTrackAfterErase...";
#endif
            while (!m_VectorTrack.empty()) {
                vector<T>* wTrack = m_VectorTrack.back();
                if (wTrack != nullptr) {
                    while (!wTrack->empty()) {
                        T wItem = wTrack->back();
                        if (wItem != T{}) {
                            return; // Exit for the fist not empty Item
                        }
                        wTrack->pop_back();
                    }
                }
                m_VectorTrack.pop_back();
                delete(wTrack);
            }
#ifdef sparsearraydebug
            cout << RealSize() << "."  << endl;
#endif
        }

        /// @brief      Visit occupied slots (value != T{}). sFn(index, value) returns false to stop.
        template<typename tFn>
        SkInline void ForEachOccupied(tFn sFn) {
            const tSize wTrackCount = m_VectorTrack.size();
            for (tSize wTrackIndex = 0; wTrackIndex < wTrackCount; ++wTrackIndex) {
                vector<T>* wTrack = m_VectorTrack[wTrackIndex];
                if (wTrack == nullptr || wTrack->empty()) {
                    continue;
                }
                const tSize wTrackSize = wTrack->size();
                for (tSize wIndex = 0; wIndex < wTrackSize; ++wIndex) {
                    T wItem = (*wTrack)[wIndex];
                    if (wItem != T{}) {
                        if (!sFn(ReturnIndice(wTrackIndex, wIndex), wItem)) {
                            return;
                        }
                    }
                }
            }
        }

        /// @brief      Visit occupied slots in [sBegin, sEnd] inclusive. Skips empty tracks.
        template<typename tFn>
        SkInline void ForEachOccupied(tSize sBegin, tSize sEnd, tFn sFn) {
            if (sBegin > sEnd || m_VectorTrack.empty()) {
                return;
            }
            tSize wFirstTrack = 0;
            tSize wFirstSlot = 0;
            tSize wLastTrack = 0;
            tSize wLastSlot = 0;
            tie(wFirstTrack, wFirstSlot) = ReturnTrackIndex(sBegin);
            tie(wLastTrack, wLastSlot) = ReturnTrackIndex(sEnd);
            if (wFirstTrack >= m_VectorTrack.size()) {
                return;
            }
            if (wLastTrack >= m_VectorTrack.size()) {
                wLastTrack = m_VectorTrack.size() - 1;
                wLastSlot = m_SizeTrack - 1;
            }
            for (tSize wTrackIndex = wFirstTrack; wTrackIndex <= wLastTrack; ++wTrackIndex) {
                vector<T>* wTrack = m_VectorTrack[wTrackIndex];
                if (wTrack == nullptr || wTrack->empty()) {
                    continue;
                }
                tSize wFrom = (wTrackIndex == wFirstTrack) ? wFirstSlot : 0;
                tSize wTo = (wTrackIndex == wLastTrack) ? wLastSlot : (m_SizeTrack - 1);
                if (wTo >= wTrack->size()) {
                    wTo = wTrack->size() - 1;
                }
                if (wFrom > wTo) {
                    continue;
                }
                for (tSize wIndex = wFrom; wIndex <= wTo; ++wIndex) {
                    T wItem = (*wTrack)[wIndex];
                    if (wItem != T{}) {
                        if (!sFn(ReturnIndice(wTrackIndex, wIndex), wItem)) {
                            return;
                        }
                    }
                }
            }
        }

        /// @brief      Visit occupied slots from sBegin to the last allocated track.
        template<typename tFn>
        SkInline void ForEachOccupiedFrom(tSize sBegin, tFn sFn) {
            if (m_VectorTrack.empty()) {
                return;
            }
            const tSize wSize = Size();
            if (wSize == 0) {
                return;
            }
            ForEachOccupied(sBegin, wSize - 1, sFn);
        }

        /// @brief      Basic method to perform call back.
        /// @param[in]  sSparseArrayCallBack SkSparseArrayCallBack<T>*
        SkInline void CallBack(tSparseArrayCallBack<T>* sSparseArrayCallBack) {
            ForEachOccupied([&](tSize /*sIndex*/, T sItem) {
                return sSparseArrayCallBack->CallBack(sItem);
            });
        }

        /// @brief      Call back occupied slots between a start and end position (inclusive).
        /// @param[in]  sSparseArrayCallBack  SkSparseArrayCallBack<T>* call back object
        /// @param[in]  sBegin tSize
        /// @param[in]  sEnd tSize
        SkInline void CallBack(tSparseArrayCallBack<T>* sSparseArrayCallBack, tSize sBegin, tSize sEnd) {
            ForEachOccupied(sBegin, sEnd, [&](tSize /*sIndex*/, T sItem) {
                return sSparseArrayCallBack->CallBack(sItem);
            });
        }

    };
    //=========================================================================
    //! Template class use for "SparseArray" pointer for T. Clear delete attached Class 
    template <class T>
    class tSparseArrayPt : public tSparseArray<T> {
    public:
        /// @brief      Constructor SkSparseArrayPt .
        tSparseArrayPt() :	tSparseArray<T>() {
        }
        
        /// @brief      Constructor SkSparseArrayPt with size of track.
        /// @param[in]  sSizeTrack tSize size of track
        tSparseArrayPt(tSize sSizeTrack) : tSparseArray<T>() {
            tSparseArray<T>::m_SizeTrack = sSizeTrack;
        }

        /// @brief      Destructor SkSparseArrayPt.
        virtual ~tSparseArrayPt() {
            Clear();
        }

        /// @brief      Clear if sDelete==true delete class.
        /// @param[in]	sDelete tBool 
        virtual void Clear(tBool sDelete=true) {
            for (auto wTrack : tSparseArray<T>::m_VectorTrack) {
                if (sDelete) {
                    if (wTrack != nullptr) {
                        for (auto wItem : *wTrack) {
                            if (wItem != nullptr) {
                                wItem->Clear();
                                delete(wItem);
                            }
                        }
                        delete(wTrack);
                    }
                }
            }
            tSparseArray<T>::m_VectorTrack.clear();
        }
        
        /// @brief      Callback while delete class.
        /// @param[in]  sIndex tSize Position
        /// @param[in]  sSize tSize length
        virtual void DeleteClass(tSize sIndex, tSize sSize) {
            // delete element
            for (tSize wIndex = sIndex; wIndex < sIndex + sSize; wIndex++) {
                T wItem =(*this)(wIndex); // (*this) else error
                if (wItem != nullptr) {
                    delete(wItem);
                    (*this)(wIndex)=nullptr;
                }
            }
        }
        
    };


} // Fin du namespace

#endif
