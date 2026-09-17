//================================================e=============================
// Skeema Allocator
/**
* @page SkAllocator
* @par
* @par Alloc class in vector of array of this class.
* @par Use for gain of memory...
*/

//! Use for gain of memory.
//=============================================================================
#ifndef SkAllocator_hpp
#define SkAllocator_hpp

#include <stdlib.h>
#include <tuple>
#include <vector>
#include "../include/SkTypes.hpp"
#include "../include/SkClass.hpp"
#include "../include/SkException.hpp"
 
namespace SkRoot {
#ifdef _DEBUGSK
#define _checkallocator
#endif

	//! Class containing the stored class with a field to indicate if the element is empty or deleted
	template <class T>
	struct alignas(SkAlign) tAllocatorEnreg {
	public:
		T		m_Elem{};
		tBool	m_Empty; 
		tAllocatorEnreg() : m_Empty(true) {}
		~tAllocatorEnreg() {}
	};

	//! Track containing T classes, not pointer for T, S integer value
	template <class T,typename S,S SizeTrack>
	class alignas(SkAlign) tAllocatorTrack : public tClass {
	private:
		tAllocatorEnreg<T>		m_Enreg[SizeTrack];
		S						m_Last;
		S						m_NbElem;
		stack<S>				m_StackDelete;
	public:
		tAllocatorTrack() : tClass(), m_Last(max_value<S>),m_NbElem(0)  {
		}
        /*
        ~tAllocatorTrack() {
        }
		*/
        /// @brief      Method Clear if if erase = true delete Item
        /// @param[in]  sDelete  tBool default = true
        void Clear(const tBool sDelete=true) {
            if (sDelete) {
                for (auto& wElem : m_Enreg) {
                    if (!wElem.m_Empty) {
                        wElem.m_Elem.Clear();
                        wElem.m_Empty=true;
                    }
                }
            }
            while (!m_StackDelete.empty()) {
                m_StackDelete.pop();
            }
            m_Last=max_value<S>;
            m_NbElem=0;
        }
         
        SkInline S Alloc() {
			S wIndex = max_value<S>; // Max S (works for both signed and unsigned)
			if (!m_StackDelete.empty()) {
				wIndex = m_StackDelete.top();
				m_StackDelete.pop();
#ifdef _checkallocator
				if (!m_Enreg[wIndex].m_Empty) {
					tStringStream wStream;
					wStream << "Allocator by Stack Element is use !";
					cout << wStream.str() << endl;
					throw(tExceptionInternalError(wStream.str()));
				}
#endif
				// Clear the element before reusing it to avoid residual data
				m_Enreg[wIndex].m_Elem.Clear();
			} else {
				if (m_Last == SizeTrack-1) {
					return(max_value<S>); // Return Max S (works for both signed and unsigned)
				}
				wIndex=++m_Last;
			}
			// Add NbElem 
			m_NbElem++;
#ifdef debugallocator
            cout << "SkAlloc :" <<  wIndex << endl;
#endif
			m_Enreg[wIndex].m_Empty = false;
			return(wIndex);
		}
		
		// sCallerRef is only printed under _checkallocator / debugallocator.
		SkInline tBool Delete(const S sIndex, [[maybe_unused]] const S sCallerRef) {
			if (sIndex >= SizeTrack) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator delete(" << sCallerRef << ") index out of bounds !";
				cout << wStream.str() << endl;
#endif
				return(false);
			}
			if (m_Enreg[sIndex].m_Empty) {
#ifdef _checkallocator
				tStringStream wStream;
                wStream << "Error: Allocator delete("<<  sCallerRef << ")  bad position element already deleted !";
				cout << wStream.str() << endl;
				throw(tExceptionInternalError(wStream.str()));
#endif
				return(false);
			}

#ifdef debugallocator
            cout << "SkDelete :" <<  sCallerRef;
#endif
			m_Enreg[sIndex].m_Elem.Clear();
			m_StackDelete.push(sIndex);
			m_Enreg[sIndex].m_Empty = true;
#ifdef debugallocator
            cout << " Ok";
#endif
			m_NbElem--;
			return(true);
		}

		// Same condition as Alloc(): no free slot on the stack and sequential cursor at the end.
		SkInline tBool Full() const {
			return(m_StackDelete.empty() && m_Last == SizeTrack - 1);
		}
	
		SkInline T* Get(const S sIndex) {
			return(const_cast<T*>(static_cast<const tAllocatorTrack*>(this)->Get(sIndex)));
		}

		/// @brief Get a const pointer to an element by its index
		/// @param[in] sIndex Index of the element
		/// @return Const pointer to the element, or nullptr if the element is empty
		SkInline const T* Get(const S sIndex) const {
			if (sIndex >= SizeTrack) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator get(" << sIndex << ") index out of bounds !";
				cout << wStream.str() << endl;
#endif
				return(nullptr);
			}
			if (m_Enreg[sIndex].m_Empty) {
#ifdef debugallocator
				cout << "SkGet :" << sIndex << " nullptr" << endl;
#endif
				return(nullptr);
			}
#ifdef debugallocator
			cout << "SkGet :" << sIndex << " Ok" << endl;
#endif
			return(&(m_Enreg[sIndex].m_Elem));
		}

        SkInline tBool IsNullptr(const S sIndex) const {
			// Check bounds (works for both signed and unsigned types)
			if (sIndex >= SizeTrack) {
				return(true);
			}
            return(m_Enreg[sIndex].m_Empty);
        }

		SkInline tBool Empty() const { return(m_NbElem == 0); }

		SkInline S NbElem() const { return(m_NbElem);  }
	};

	//=========================================================================
	//! Allocator Root "Contains the tracks". T must have Clear() method. Example : SkAllocator<SkItem,tInt,1024>
	template <class T, typename S, tInt SizeTrack>
	class alignas(SkAlign) tAllocator : tClass {
	private:
		typedef vector< tAllocatorTrack<T, S, SizeTrack>*> tVectorAllocatorTrack;
		// Vector of allocator track
		tVectorAllocatorTrack m_AllocatorTrack;
		S m_Last;
		stack<S> m_StackDelete;
	public:
		/// @brief Constructor
		tAllocator() : tClass() , m_Last(0) {
			Init();
		}
        
		/// @brief Destructor
		~tAllocator() {
			Clear(false);
		}
        
		/// @brief Clear all tracks and optionally delete elements
		/// @param[in] sDelete If true, delete elements
		void Clear(const tBool sDelete=true) {
			for (auto wTrack : m_AllocatorTrack) {
                if (wTrack != nullptr) {
                    wTrack->Clear(sDelete);
                    delete(wTrack);
                }
			}
			m_AllocatorTrack.clear();
			m_Last = 0;
            while (!m_StackDelete.empty()) {
                m_StackDelete.pop();
            }
		}
        
		/// @brief Initialize the allocator by allocating the first track
		void Init() {
			// Alloc first track
			m_AllocatorTrack.push_back(new tAllocatorTrack<T, S, SizeTrack>());
		}
	private:
		/// @brief Return the track index and index within the track for a given global index
		/// @param[in] sIndex Global index of the element
		/// @return Tuple containing the track index and index within the track
		SkInline void ReturnTrackIndex(const S sIndex, S& sTrackIndex, S& sIndexTrack) const {
			sTrackIndex = sIndex / SizeTrack;
			sIndexTrack = sIndex % SizeTrack;
		}

		/// @brief Return the global index for a given track index and index within the track
		/// @param[in] sTrack Track index
		/// @param[in] sIndexTrack Index within the track
		/// @return Global index
		SkInline S ReturnIndice(const S sTrack, const S sIndexTrack) const {
			return(SizeTrack * sTrack + sIndexTrack); 
		}

		/// @brief Allocate a new track or reuse an existing non-full track
		/// @return Index of the track
		S NewTrack() {
			// First, try to reuse a deleted track slot
			if (!m_StackDelete.empty()) {
				m_Last = m_StackDelete.top();
				m_AllocatorTrack[m_Last] = new tAllocatorTrack<T, S, SizeTrack>();
				m_StackDelete.pop();
				return(m_Last);
			}
			// Search for an existing non-full track
			for (S i = 0; i < m_AllocatorTrack.size(); i++) {
				if (m_AllocatorTrack[i] != nullptr) {
					if (!m_AllocatorTrack[i]->Full()) {
						m_Last = i;
						return(i);
					}
				}
			}
			// No available track found, create a new one
#ifdef debugallocator
            cout << "SkNewTrack:"  << endl;
#endif
			m_AllocatorTrack.push_back(new tAllocatorTrack<T, S, SizeTrack>());
			m_Last = S(m_AllocatorTrack.size()) - 1;            
			return(m_Last);
		}
		
	public:
		/// @brief Allocate a new element
		/// @return Tuple containing the global index and pointer to the new element
		SkInline tuple<S, T*>  Alloc() {
            if (m_AllocatorTrack.empty()) {
                Init();
            }
			if (m_AllocatorTrack[m_Last] == nullptr) {
				m_Last = NewTrack();
			}

			S wIndex = m_AllocatorTrack[m_Last]->Alloc();
			
			if (wIndex == max_value<S>) {
				m_Last = NewTrack();
				wIndex = m_AllocatorTrack[m_Last]->Alloc();
				if (wIndex == max_value<S>) {
					return(make_tuple(max_value<S>, nullptr));
				}
			}

			tAllocatorTrack<T, S, SizeTrack>* const wTrack = m_AllocatorTrack[m_Last];
			return(make_tuple(ReturnIndice(m_Last,wIndex)+1, wTrack->Get(wIndex))); // +1 never 0
		}

		/// @brief Delete an element by its global index
		/// @param[in] sIndex Global index of the element
		/// @return True if the element was successfully deleted, false otherwise
		tBool Delete(const S sIndex) {
			if (sIndex == 0) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator delete(" << sIndex << ") index Nullptr !";
				cout << wStream.str() << endl;
#endif
				return(false);
			}
			const S wInternal = sIndex - 1;
			S wTrackIndex;
			S wIndex;
			ReturnTrackIndex(wInternal, wTrackIndex, wIndex);
			if (wTrackIndex >= m_AllocatorTrack.size()) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator delete(" << sIndex << ") index out of bounds !";
				cout << wStream.str() << endl;
#endif
				return(false);
			}
			if (m_AllocatorTrack[wTrackIndex] == nullptr) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator delete(" << sIndex << ") index nullptr !";
				cout << wStream.str() << endl;
#endif
				return(false);
			}
			if (!m_AllocatorTrack[wTrackIndex]->Delete(wIndex, sIndex)) {
#ifdef _checkallocator
				tStringStream wStream;
				wStream << "Error: Allocator delete(" << sIndex << ") index delete failed !";
				cout << wStream.str() << endl;
#endif
				return(false);
			}

			if (m_AllocatorTrack[wTrackIndex]->Empty()) {
				delete(m_AllocatorTrack[wTrackIndex]);
				m_AllocatorTrack[wTrackIndex] = nullptr;
				m_StackDelete.push(wTrackIndex);
			}
			return(true);
		}

		/// @brief Get a pointer to an element by its global index
		/// @param[in] sIndex Global index of the element
		/// @return Pointer to the element, or nullptr if the element does not exist
		SkInline T* operator ()(const S sIndex) {
#ifdef debugallocator
            cout << "SK(" << sIndex << ")";
#endif
			return(const_cast<T*>(static_cast<const tAllocator*>(this)->operator()(sIndex)));
		}

		/// @brief Get a const pointer to an element by its global index
		/// @param[in] sIndex Global index of the element
		/// @return Const pointer to the element, or nullptr if the element does not exist
		SkInline const T* operator ()(const S sIndex) const {
#ifdef debugallocator
            if (sIndex == 0) {
                cout << " nullptr" << endl;
            } else {
                cout << endl;
            }
#endif
			if (sIndex == 0) {
				return(nullptr);
			}
			const S wInternal = sIndex - 1;
			S wTrackIndex;
			S wIndex;
			ReturnTrackIndex(wInternal, wTrackIndex, wIndex);
			if (wTrackIndex >= m_AllocatorTrack.size()) {
				return(nullptr);
			}
			if (m_AllocatorTrack[wTrackIndex] == nullptr) {
				return(nullptr);
			}
			return(m_AllocatorTrack[wTrackIndex]->Get(wIndex));
		}

        /// @brief Check if an element is null by its global index
        /// @param[in] sIndex Global index of the element
        /// @return True if the element is null, false otherwise
        SkInline tBool IsNullptr(const S sIndex) const {
            if (sIndex == 0) {
				return(true);
			}
            const S wInternal = sIndex - 1;
            S wTrackIndex;
            S wIndex;
            ReturnTrackIndex(wInternal, wTrackIndex, wIndex);
            if (wTrackIndex >= m_AllocatorTrack.size()) {
				return(true);
			}
            if (m_AllocatorTrack[wTrackIndex] == nullptr) {
				return(true);
			}
            return(m_AllocatorTrack[wTrackIndex]->IsNullptr(wIndex));
        }

		/// @brief Return the number of elements
		/// @return Number of elements
		S Size() const {
			S wResult = 0;
			for (S i = 0; i < m_AllocatorTrack.size(); i++) {
				if (m_AllocatorTrack[i] != nullptr)  wResult+=m_AllocatorTrack[i]->NbElem();
			}
			return(wResult);
		}

		/// @brief Return the total size of memory used
		/// @return Total size of memory used
		tLongLong MemorySize() const {
			tLongLong wResult = 0;
			const tLongLong wSizeOf = sizeof(tAllocatorTrack<T, S, SizeTrack>);
			for (S i = 0; i < m_AllocatorTrack.size(); i++) {
				if (m_AllocatorTrack[i] != nullptr)  wResult += wSizeOf;
			}
			return(wResult);
		}
	};
} // end of namespape

#endif
