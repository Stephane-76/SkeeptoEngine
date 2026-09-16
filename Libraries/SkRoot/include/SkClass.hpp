//=============================================================================
// SkRoot tClass
/**
 * @page tClass 
 * @par 
 * @par tClass is base class of all class of library and program (Class ancestor of all classes).
 * @par tVirtualClass is used for derived class with cloning method.
 * @par The tVariant Class can contains a SkVirtual Class.
 * @par tVariant with virtual class can apply operator +,-,* and /.
 */
 //! Class ancestor of all classes
//=============================================================================
#ifndef SkClass_hpp
#define SkClass_hpp

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "SkTypes.hpp"

namespace SkRoot {
    //=========================================================================
    //! Ancestor Class for all program SkRoot 
    class tClass {
    protected:
#ifdef _DEBUGLeak
        int m_IndiceAlloc;
#endif
    public:
        /// @brief Constructor
        tClass();
        
        /// @brief Copy constructor
        /// @param[in] sClass Reference to the class to copy
        tClass(const tClass& sClass);

#ifdef _DEBUG	
        virtual
#endif
        /// @brief Destructor
        ~tClass();

#ifdef _DEBUGLeak
        /// @brief Get the number of allocations
        /// @return Number of allocations
        int GetNbAlloc();
#endif
    };

    class tVariant;
    //=========================================================================
    //! Virtual Class for variant and Factory class
    class tVirtualClass : public tClass {
    public:
        /// @brief Constructor for virtual class
        tVirtualClass();
        
        /// @brief Copy constructor for virtual class
        /// @param[in] sVirtualClass Reference to the virtual class to copy
        tVirtualClass(const tVirtualClass& sVirtualClass);
        
        /// @brief Destructor for virtual class
        virtual ~tVirtualClass();

        /// @brief Check if the class is a copy
        /// @return True if the class is a copy, false otherwise
        virtual tBool IsCopy();

        /// @brief Clone the derived class
        /// @return Pointer to the cloned class
        virtual tVirtualClass* Clone();

        /// @brief Get the name of the class (for factory)
        /// @return Name of the class
        virtual tString ClassName() const;

        // for Variant (Class) ================================================
        /// @brief Check if calculation propagation is enabled
        /// @return True if calculation propagation is enabled, false otherwise
        virtual tBool IsCalculationPropagation() const;
        
        /// @brief Set the unique variant for the spreadsheet
        /// @param[in] sValue Pointer to the variant
        virtual void Value(tVariant* sValue);
        
        /// @brief Get the unique variant for the spreadsheet
        /// @return Pointer to the variant
        virtual tVariant* Value();

        // Json ===============================================================
        /// @brief Write the class to JSON
        /// @param[in] sWriter Pointer to the JSON writer
        virtual void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

        /// @brief Read the class from JSON
        /// @param[in] sValue Reference to the JSON value
        virtual void Json(const rapidjson::Value& sValue);
        
        // React =============================================================
        /// @brief Write the class to JSON for JavaScript
        /// @param[in] sWriter Pointer to the JSON writer
        virtual void JsonJavaScript(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

        /// @brief Check if JSON generation for JavaScript is enabled
        /// @return True if JSON generation for JavaScript is enabled, false otherwise
        virtual tBool IsJsonJavaScript();
        
        /// @brief Check if the class is a React component
        /// @return True if the class is a React component, false otherwise
        virtual tBool IsReactComponent();
        
        // for Variant (Class) ================================================
        /// @brief + operator for the spreadsheet
        /// @param[in] sLeft Boolean indicating if the operation is left-sided
        /// @param[in] sVariant Reference to the variant
        /// @return Result of the operation
        virtual tVariant Operator_plus(tBool sLeft, const tVariant& sVariant);
        
        /// @brief - operator for the spreadsheet
        /// @param[in] sLeft Boolean indicating if the operation is left-sided
        /// @param[in] sVariant Reference to the variant
        /// @return Result of the operation
        virtual tVariant Operator_minus(tBool sLeft, const tVariant& sVariant);
        
        /// @brief * operator for the spreadsheet
        /// @param[in] sLeft Boolean indicating if the operation is left-sided
        /// @param[in] sVariant Reference to the variant
        /// @return Result of the operation
        virtual tVariant Operator_multiply(tBool sLeft, const tVariant& sVariant);
        
        /// @brief / operator for the spreadsheet
        /// @param[in] sLeft Boolean indicating if the operation is left-sided
        /// @param[in] sVariant Reference to the variant
        /// @return Result of the operation
        virtual tVariant Operator_divide(tBool sLeft, const tVariant& sVariant);

        /// @brief Equality operator
        /// @param[in] sVirtualClass Reference to the virtual class to compare
        /// @return True if the classes are equal, false otherwise
        virtual tBool operator==(const tVirtualClass& sVirtualClass) const;
        
#ifdef _DEBUGSK
        /// @brief Get debug information
        /// @return Debug information as a string
        virtual tString Debug();
#endif
    };

    //! Class to manage a bit set ================================
    template <class T>
    class tBitSet {
    private:
        T m_Bits;
    public:
        /// @brief Constructor
        tBitSet() : m_Bits(0) {}

        /// @brief Copy constructor
        /// @param[in] sBitSet Reference to the bit set to copy
        tBitSet(const tBitSet& sBitSet) : m_Bits(sBitSet.m_Bits) {}

        /// @brief Constructor
        /// @param[in] sValue Value to set the bit set
        tBitSet(const T& sValue) : m_Bits(sValue) {}

        /// @brief Set the bit at the position pos
        /// @param[in] sPos Position to set the bit
        void Set(T sPos) {
            m_Bits |= (1 << sPos);
        }

        /// @brief Clear the bit at the position pos
        /// @param[in] sPos Position to clear the bit
        void Clear(T sPos) {
            m_Bits &= ~(1 << sPos);
        }

        /// @brief Clear all bits
        void ClearAll() {
            m_Bits = 0;
        }
        
        /// @brief return True if empty
        /// @return True if m_bits==0
        tBool Empty() const {
            return(m_Bits==0);
        }

        /// @brief Check if the bit at the position pos is set
        /// @param[in] sPos Position to check the bit
        /// @return True if the bit is set, false otherwise
        bool Value(T sPos) const {
            return m_Bits & (1 << sPos);
        }

        /// @brief Toggle the bit at the position pos
        /// @param[in] sPos Position to toggle the bit
        void toggle(T sPos) {
            m_Bits ^= (1 << sPos);
        }

        /// @brief () operator
        /// @return Int <T> Value
        T operator ()() {
            return m_Bits;
        }

        /// @brief Assignment operator
        /// @param[in] sBits Reference to Int <T> Value
        tBitSet& operator =(const T& sBits) {
            m_Bits = sBits;
            return *this;
        }


        /// @brief Assignment operator
        /// @param[in] sBitSet Reference to the bit set to assign
        tBitSet& operator =(const tBitSet& sBitSet) {
            m_Bits = sBitSet.m_Bits;
            return *this;
        }

        /// @brief Equality operator
        /// @param[in] sBitSet Reference to the bit set to compare
        /// @return True if the bit sets are equal, false otherwise
        bool operator ==(const tBitSet& sBitSet) const {
            return m_Bits == sBitSet.m_Bits;
        }

        /// @brief Not equal operator
        /// @param[in] sBitSet Reference to the bit set to compare
        /// @return True if the bit sets are not equal, false otherwise
        bool operator !=(const tBitSet& sBitSet) const {    
            return m_Bits != sBitSet.m_Bits;
        }

        /// @brief Merge this bit set with another bit set using OR operation
        /// @param[in] sBitSet Reference to the bit set to merge with
        /// @return Reference to this bit set after merging
        tBitSet& Merge(const tBitSet& sBitSet) {
            m_Bits |= sBitSet.m_Bits;
            return *this;
        }

        /// @brief Merge this bit set with another bit set using OR operation
        /// @param[in] sBitSet Reference to the bit set to merge with
        /// @return New bit set containing the merged result
        tBitSet MergeCopy(const tBitSet& sBitSet) const {
            return tBitSet(m_Bits | sBitSet.m_Bits);
        }

        /// @brief Return BitSet 
        T BitSet() const {
            return m_Bits;
        }

        /// @brief Set BitSet
        /// @param[in] sBits Value to set the bit set
       void BitSet(T sBits) {
            m_Bits = sBits;
        }

        /// @brief Display the state of all bits (for demonstration)
        void Debug() const {
            std::cout << "Current bits: ";
            for (int i = static_cast<int>(sizeof(T) * 8) - 1; i >= 0; --i) {
                std::cout << ((m_Bits & (static_cast<T>(1) << i)) ? '1' : '0');
            }
            std::cout << std::endl;
        }
    };


    //! Class comparator to sort in order of pointers (and preserve uniqueness)
    template <class T>
    struct tComparatorClass {
        inline bool operator()(T E1, T E2) { return (E1 < E2); }
    };

    //==========================================================================
    template <class T>
    //! Contains unique class ordered by operator < (preserves the uniqueness of T class). 
    class tClassVector {
    public:
        typedef vector<T> tContainerClass;
        typedef vector<T> tResult;
        typedef typename tContainerClass::iterator tIteratorClass;
    private:
        tContainerClass m_Vector;

        /// @brief Append the intersection of two sorted unique vectors into sResult.
        /// Both inputs stay ordered by operator < (same invariant as InsertClass).
        /// Uses an advancing binary search when one side is much smaller (Pressure3:
        /// ~10 row ranges vs ~100k column ranges), otherwise a linear merge.
        static inline void IntersectSorted(const tContainerClass& sSmall, const tContainerClass& sLarge, tResult* sResult) {
            const size_t n = sSmall.size();
            const size_t m = sLarge.size();
            if (n == 0 || m == 0) {
                return;
            }
            sResult->reserve(sResult->size() + n);

            size_t wLogLarge = 1;
            for (size_t w = m; w > 1; w >>= 1) {
                ++wLogLarge;
            }
            // n * log2(m) vs n + m: keep binary search when the left set is tiny.
            if (n * wLogLarge + 16 < n + m) {
                tComparatorClass<T> wCmp;
                auto wLo = sLarge.begin();
                const auto wEnd = sLarge.end();
                for (const T& wElement : sSmall) {
                    wLo = std::lower_bound(wLo, wEnd, wElement, wCmp);
                    if (wLo == wEnd) {
                        break;
                    }
                    if (wElement == *wLo) {
                        sResult->push_back(*wLo);
                        ++wLo;
                    }
                }
            }
            else {
                size_t i = 0;
                size_t j = 0;
                while (i < n && j < m) {
                    const T& wA = sSmall[i];
                    const T& wB = sLarge[j];
                    if (wA < wB) {
                        ++i;
                    }
                    else if (wB < wA) {
                        ++j;
                    }
                    else {
                        sResult->push_back(wA);
                        ++i;
                        ++j;
                    }
                }
            }
        }

    public:
        /// @brief Constructor
        tClassVector() { m_Vector.resize(0); m_Vector.shrink_to_fit(); }
        
        /// @brief Destructor
        ~tClassVector() { m_Vector.clear(); }

        /// @brief Insert class in vector
        /// @param[in] sClass Class to insert
        /// @return False if the class already exists, true otherwise
        inline tBool InsertClass(T sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, tComparatorClass<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere != sClass) {
                    m_Vector.insert(wWhere, sClass);
                }
                else {
                    // Exist
                    return(false);
                }
            }
            else {
                // Insert at end 
                m_Vector.insert(wWhere, sClass);
            }
            return(true);
        }

        /// @brief Find class in vector
        /// @param[in] sClass Class to find
        /// @return Iterator to the class if found, end iterator otherwise
        inline tIteratorClass FindClass(T sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, tComparatorClass<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere == sClass) {
                    return(wWhere);
                }
            }
            return(m_Vector.end());
        }

        /// @brief Delete class from vector
        /// @param[in] sClass Class to delete
        /// @return False if the class does not exist, true otherwise
        inline tBool DeleteClass(T sClass) {
            tIteratorClass wIterator = FindClass(sClass);
            if (wIterator != m_Vector.end()) {
                m_Vector.erase(wIterator);
                return(true);
            }
            return(false);
        }

        /// @brief Return pointer to the vector
        /// @return Pointer to the vector
        inline tContainerClass* Container() {
            return(&m_Vector);
        }
        
        inline const tContainerClass* Container() const {
            return(&m_Vector);
        }

        /// @brief Check if the class exists in the vector
        /// @param[in] sClass Class to check
        /// @return True if the class exists, false otherwise
        inline tBool Exist(T sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, tComparatorClass<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere==sClass) {
                    return(true);
                }
            }
            return(false);
        }

        /// @brief Return container class with intersect value
        /// @param[in] sClassContainer Pointer to the class container
        /// @param[out] sResult Pointer to the result container
        inline void GetIntersection(tClassVector<T>* sClassContainer, tResult* sResult) {
            if (sClassContainer == nullptr || sResult == nullptr) {
                return;
            }
            if (m_Vector.size() <= sClassContainer->m_Vector.size()) {
                IntersectSorted(m_Vector, sClassContainer->m_Vector, sResult);
            }
            else {
                IntersectSorted(sClassContainer->m_Vector, m_Vector, sResult);
            }
        }

        /// @brief Resize the vector
        inline void Resize() {
            if (m_Vector.capacity() - m_Vector.size() > 3)	m_Vector.shrink_to_fit();
        }

        /// @brief Assignment operator
        /// @param[in] sClassVector Reference to the class vector to assign
        tClassVector& operator = (const tClassVector& sClassVector) {
            m_Vector = sClassVector.m_Vector;
            return *this;
        }
        
        /// @brief Equality operator
        /// @param[in] sClassVector Reference to the class vector to compare
        /// @return True if the vectors are equal, false otherwise
        tBool operator == (const tClassVector<T>& sClassVector) const {
            return(m_Vector == sClassVector.m_Vector);
        }
    };

    //==========================================================================
    template <class T>
    //! Contains unique class ordered by value (preserves the uniqueness of T class). 
    class tClassSet {
    public:
        typedef set<T> tContainerClass;
        typedef vector<T> SkResult;
        typedef typename tContainerClass::iterator tIteratorClass;
    private:
        tContainerClass m_Set;
    public:
        /// @brief Constructor
        tClassSet() { }
        
        /// @brief Destructor
        ~tClassSet() { m_Set.clear(); }

        /// @brief Insert class in set
        /// @param[in] sClass Class to insert
        /// @return False if the class already exists, true otherwise
        inline tBool InsertClass(T sClass) {
            if (Exist(sClass)) return(false);
            m_Set.insert(sClass);
            return(true);
        }

        /// @brief Find class in set
        /// @param[in] sClass Class to find
        /// @return Iterator to the class if found, end iterator otherwise
        inline tIteratorClass FindClass(T sClass) {
            tIteratorClass wWhere;
            wWhere = m_Set.find(sClass);
            if (wWhere != m_Set.end()) {
                return(wWhere);
            }
            return(m_Set.end());
        }

        /// @brief Delete class from set
        /// @param[in] sClass Class to delete
        /// @return False if the class does not exist, true otherwise
        tBool DeleteClass(T sClass) {
            tIteratorClass wIterator = FindClass(sClass);
            if (wIterator != m_Set.end()) {
                m_Set.erase(wIterator);
                return(true);
            }
            return(false);
        }

        /// @brief Return pointer to the set
        /// @return Pointer to the set
        tContainerClass* Container() {
            return(&m_Set);
        }

        /// @brief Check if the class exists in the set
        /// @param[in] sClass Class to check
        /// @return True if the class exists, false otherwise
        inline tBool Exist(T sClass) {
            tIteratorClass wWhere;
            wWhere = m_Set.find(sClass);
            return(wWhere != m_Set.end());
        }

        /// @brief Return container class with intersect value
        /// @param[in] sClassContainer Pointer to the class container
        /// @param[out] sResult Pointer to the result container
        inline void GetIntersection(tClassSet<T>* sClassContainer, SkResult* sResult) {
            if (m_Set.size() < sClassContainer->Container()->size()) {
                for (tIteratorClass wIterator = m_Set.begin(); wIterator != m_Set.end(); wIterator++) {
                    T wElement = *wIterator;
                    tIteratorClass wWhere;
                    wWhere = sClassContainer->FindClass(wElement);
                    if (wWhere != sClassContainer->Container()->end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
            else {
                for (tIteratorClass wIterator = sClassContainer->Container()->begin(); wIterator != sClassContainer->Container()->end(); wIterator++) {
                    T wElement = *wIterator;
                    tIteratorClass wWhere;
                    wWhere = FindClass(wElement);
                    if (wWhere != m_Set.end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
        }

        /// @brief Equality operator
        /// @param[in] sClassSet Reference to the class set to compare
        /// @return True if the sets are equal, false otherwise
        tBool operator == (const tClassSet<T>& sClassSet) const {
            if (m_Set.size() != sClassSet.m_Set.size()) return(false);
            
            tIteratorClass wIteratorRef = sClassSet.m_Set.begin();
            for (tIteratorClass wIterator = m_Set.begin(); wIterator != m_Set.end(); wIterator++) {
                if (*wIterator != *wIteratorRef) return(false);
                wIteratorRef++;
            }
            return(true);
        }
    };

    //! Class comparator to sort in order of pointers (and preserve uniqueness)
    template <class T>
    struct SkComparatorClassPt {
        inline bool operator()(T* E1, T* E2) { return (E1 < E2); }
    };

    //==========================================================================
    template <class T>
    //! Contains unique class ordered by pointer. (preserves the uniqueness of T class). Used for leak memory among others 
    class tClassVectorContainer  {
    public:
        typedef vector<T*> tContainerClass;
        typedef typename tContainerClass::iterator tIteratorClass;
    private:
        tContainerClass m_Vector;
    public:
        /// @brief Constructor
        tClassVectorContainer() { m_Vector.resize(0); }
        
        /// @brief Destructor
        ~tClassVectorContainer() { m_Vector.clear(); }

        /// @brief Insert class in vector
        /// @param[in] sClass Pointer to the class to insert
        /// @return False if the class already exists, true otherwise
        inline tBool InsertClass(T* sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, SkComparatorClassPt<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere != sClass) {
                    m_Vector.insert(wWhere, sClass);
                } else {
                    // Exist
                    return(false);
                }
            }
            else {
                // Insert at end 
                m_Vector.insert(wWhere, sClass);
            }
            return(true);
        }

        /// @brief Find class in vector
        /// @param[in] sClass Pointer to the class to find
        /// @return Iterator to the class if found, end iterator otherwise
        inline tIteratorClass FindClass(T* sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, SkComparatorClassPt<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere == sClass) {
                    return(wWhere);
                }
            }
            return(m_Vector.end());
        }

        /// @brief Delete class from vector
        /// @param[in] sClass Pointer to the class to delete
        /// @return False if the class does not exist, true otherwise
        inline tBool DeleteClass(T* sClass) {
            tIteratorClass wIterator = FindClass(sClass);
            if (wIterator != m_Vector.end()) {
                m_Vector.erase(wIterator);
                return(true);
            }
            return(false);
        }

        /// @brief Return pointer to the vector
        /// @return Pointer to the vector
        inline tContainerClass* Container() {
            return(&m_Vector);
        }
        
        inline const tContainerClass* Container() const {
            return(&m_Vector);
        }

        /// @brief Check if the class exists in the vector
        /// @param[in] sClass Pointer to the class to check
        /// @return True if the class exists, false otherwise
        inline tBool Exist(T* sClass) {
            tIteratorClass wWhere;
            wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), sClass, SkComparatorClassPt<T>());
            if (wWhere != m_Vector.end()) {
                if (*wWhere==sClass) {
                    return(true);
                }
            }
            return(false);
        }
         
        /// @brief Return container class with intersect value
        /// @param[in] sClassContainer Pointer to the class container
        /// @param[out] sResult Pointer to the result container
        inline void GetIntersection(tClassVectorContainer<T>* sClassContainer,tContainerClass* sResult) {
            if (m_Vector.size() < sClassContainer->m_Vector.size()) {
                for (tIteratorClass wIterator = m_Vector.begin(); wIterator != m_Vector.end(); wIterator++) {
                    T* wPointer = *wIterator;
                    tIteratorClass wWhere;
                    wWhere = std::lower_bound(sClassContainer->m_Vector.begin(), sClassContainer->m_Vector.end(), wPointer, SkComparatorClassPt<T>());
                    if (wWhere != sClassContainer->m_Vector.end() && *wWhere == wPointer) {
                        sResult->push_back(*wWhere);
                    }
                }
            } else {
                for (tIteratorClass wIterator = sClassContainer->m_Vector.begin(); wIterator != sClassContainer->m_Vector.end(); wIterator++) {
                    T* wPointer = *wIterator;
                    tIteratorClass wWhere;
                    wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), wPointer, SkComparatorClassPt<T>());
                    if (wWhere != m_Vector.end() && *wWhere == wPointer) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
        }

        /// @brief Resize the vector
        inline void Resize() {
            if (m_Vector.capacity()-m_Vector.size()>10)	m_Vector.shrink_to_fit();
        }

        /// @brief Equality operator
        /// @param[in] sClassVectorContainer Reference to the class vector container to compare
        /// @return True if the containers are equal, false otherwise
        tBool operator == (const tClassVectorContainer<T>& sClassVectorContainer) const {
            if (m_Vector.size() != sClassVectorContainer.m_Vector.size()) return(false);

            typename tContainerClass::const_iterator wIteratorRef = sClassVectorContainer.m_Vector.begin();
            for (typename tContainerClass::const_iterator wIterator = m_Vector.begin(); wIterator < m_Vector.end(); wIterator++) {
                if ((**wIterator) != (**wIteratorRef)) return(false);
                wIteratorRef++;
            }
            return(true);
        }

        void DebugApplicationMemory();
    };
    
    
    //==========================================================================
    template <class T>
    //! Contains unique class unordered_set  by pointer. (preserves the uniqueness of T class). Used for leak memory among others 
    class tClassUnorderedContainer {
    public:
        typedef unordered_set<T*> tContainerClass;
        typedef typename tContainerClass::iterator tIteratorClass;
        typedef vector<T*> tVectorResult;
    private:
        tContainerClass m_Unordered_set;

        /// @brief      FindClass by iterator
        /// @param[in]  sClass T* pointer of class T
        /// @return     tIteratorClass vector<T*>::iterator 
        inline tIteratorClass FindClassByIterator(T* sClass) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sClass);
            return(wWhere);
        }

    public:
        tClassUnorderedContainer() { }
        ~tClassUnorderedContainer() { m_Unordered_set.clear(); }

        /// @brief      InsertClass in vector
        /// @param[in]  sClass T*  pointer of class T
        /// @return     tBool false already exist
        inline tBool InsertClass(T* sClass) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sClass);
            if (wWhere == m_Unordered_set.end()) {
                m_Unordered_set.insert(sClass);
                return(true);
            }
            return(false);
        }

        /// @brief      FindClass true of false (check)
        /// @param[in]  sClass T* pointer of class T
        /// @return     tIteratorClass vector<T*>::iterator 
        inline tBool FindClass(T* sClass) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sClass);
            return(wWhere!= m_Unordered_set.end());
        }
        
        /// @brief      DeleteClass
        /// @param[in]  sClass T* pointer of class T
        /// @return     tBool false don't exist
        inline tBool DeleteClass(T* sClass) {
            tIteratorClass wIterator = FindClassByIterator(sClass);
            if (wIterator != m_Unordered_set.end()) {
                m_Unordered_set.erase(wIterator);
                return(true);
            }
            return(false);
        }

        /// @brief      Return unordered_set<T>
        /// @return     tVectorClass* return pointer on vector
        inline tContainerClass* Container() {
            return(&m_Unordered_set);
        }

        /// @brief      Return ContainerClass with Intersect value.
        /// @param[in]	sClassUnorderedContainer tClassUnorderedContainer<T>*
        /// @param[out]	sResult tContainerClass*
        inline void GetIntersection(tClassUnorderedContainer<T>* sClassUnorderedContainer, tVectorResult* sResult) {
            if (m_Unordered_set.size() < sClassUnorderedContainer->m_Unordered_set.size()) {
                for (auto wClass : m_Unordered_set) {
                    tIteratorClass wWhere = sClassUnorderedContainer->m_Unordered_set.find(wClass);
                    if (wWhere != sClassUnorderedContainer->m_Unordered_set.end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
            else {
                for (auto wClass : sClassUnorderedContainer->m_Unordered_set) {
                    tIteratorClass wWhere = m_Unordered_set.find(wClass);
                    if (wWhere != m_Unordered_set.end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
        }

        tBool operator == (const tClassUnorderedContainer<T>& sClassUnorderedContainer) const {
            if (m_Unordered_set.size() != sClassUnorderedContainer.m_Unordered_set.size()) return(false);

            tIteratorClass wIteratorRef = sClassUnorderedContainer.m_Unordered_set.begin();
            for (tIteratorClass wIterator = m_Unordered_set.begin(); wIterator != m_Unordered_set.end(); wIterator++) {
                if ((**wIterator) != (**wIteratorRef)) return(false);
                wIteratorRef++;
            }
            return(true);
        }


        void DebugApplicationMemory();
    };

    //==========================================================================
    template <class T>
    //! Contains unique T. (preserves the uniqueness of T). 
    class tUnorderedContainer {
    public:
        typedef unordered_set<T> tContainerClass;
        typedef typename tContainerClass::iterator tIteratorClass;
        typedef vector<T> tVectorResult;
    private:
        tContainerClass m_Unordered_set;

        /// @brief Find class by iterator
        /// @param[in] sValue Value to find
        /// @return Iterator to the value if found, end iterator otherwise
        inline tIteratorClass FindByIterator(T sValue) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sValue);
            return(wWhere);
        }

    public:
        /// @brief Constructor
        tUnorderedContainer() { }
        
        /// @brief Destructor
        ~tUnorderedContainer() { m_Unordered_set.clear(); }

        /// @brief Insert value in unordered set
        /// @param[in] sValue Value to insert
        /// @return False if the value already exists, true otherwise
        inline tBool Insert(T sValue) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sValue);
            if (wWhere == m_Unordered_set.end()) {
                m_Unordered_set.insert(sValue);
                return(true);
            }
            return(false);
        }

        /// @brief Find value in unordered set
        /// @param[in] sValue Value to find
        /// @return True if the value exists, false otherwise
        inline tBool Find(T sValue) {
            tIteratorClass wWhere;
            wWhere = m_Unordered_set.find(sValue);
            return(wWhere != m_Unordered_set.end());
        }

        /// @brief Delete value from unordered set
        /// @param[in] sValue Value to delete
        /// @return False if the value does not exist, true otherwise
        inline tBool Delete(T sValue) {
            tIteratorClass wIterator = FindByIterator(sValue);
            if (wIterator != m_Unordered_set.end()) {
                m_Unordered_set.erase(wIterator);
                return(true);
            }
            return(false);
        }

        /// @brief Return pointer to the unordered set
        /// @return Pointer to the unordered set
        inline tContainerClass* Container() {
            return(&m_Unordered_set);
        }

        inline const tContainerClass* Container() const {
            return(&m_Unordered_set);
        }

        /// @brief Return container class with intersect value
        /// @param[in] sClassUnorderedContainer Pointer to the class container
        /// @param[out] sResult Pointer to the result container
        inline void GetIntersection(tUnorderedContainer<T>* sClassUnorderedContainer, tVectorResult* sResult) {
            if (m_Unordered_set.size() < sClassUnorderedContainer->m_Unordered_set.size()) {
                for (auto wClass : m_Unordered_set) {
                    tIteratorClass wWhere = sClassUnorderedContainer->m_Unordered_set.find(wClass);
                    if (wWhere != sClassUnorderedContainer->m_Unordered_set.end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
            else {
                for (auto wClass : sClassUnorderedContainer->m_Unordered_set) {
                    tIteratorClass wWhere = m_Unordered_set.find(wClass);
                    if (wWhere != m_Unordered_set.end()) {
                        sResult->push_back(*wWhere);
                    }
                }
            }
        }

        /// @brief Equality operator
        /// @param[in] sUnorderedContainer Reference to the unordered container to compare
        /// @return True if the containers are equal, false otherwise
        tBool operator == (const tUnorderedContainer<T>& sUnorderedContainer) const {
            if (m_Unordered_set.size() != sUnorderedContainer.m_Unordered_set.size()) return(false);

            tIteratorClass wIteratorRef = sUnorderedContainer.m_Unordered_set.begin();
            for (tIteratorClass wIterator = m_Unordered_set.begin(); wIterator != m_Unordered_set.end(); wIterator++) {
                if ((*wIterator) != (*wIteratorRef)) return(false);
                wIteratorRef++;
            }
            return(true);
        }
    };

   

#ifdef _DEBUGLeak
    void DebugMemory();
#endif

}; // end of namespace ========================================================
#endif
