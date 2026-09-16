//=============================================================================
// SkItemCss Alloc on vecor
// 
//
//=============================================================================
#ifndef SkItemCss_hpp
#define SkItemCss_hpp

#include <SkAllocator.hpp>
#include <SkFormatApi.hpp>

using namespace SkRoot;

namespace SkFormat {

	typedef vector<tFormatRef>	SkVectorItem;
	typedef SkVectorItem::iterator  SkIteratorItem;

	//! Class comparator to sort in order of pointers (and preserve uniqueness)
	template <class T, tInt SizeTrack>
	struct tComparatorItemCss {
	private:
		typedef tAllocator<T, tFormatRef, SizeTrack>* tAllocatorItemCss;
		tAllocatorItemCss m_Allocator;
		T* m_Current;

	public:
		/// @brief		Comparator
		/// @param[in]	sAllocator tAllocatorItemCss
		/// @param[in]	sItemCss T*
		tComparatorItemCss(tAllocatorItemCss  sAllocator, T* sItemCss) {
			m_Allocator = sAllocator;
			m_Current = sItemCss;
		}

		/// @brief		Comparator for lower_bound  
		/// @param[in]	E1 tFormatRef 
		/// @param[in]	E2 tFormatRef 
		inline bool operator()(tFormatRef E1, tFormatRef E2) {
			T* wItemCss1 = (*m_Allocator)(E1);
			if (wItemCss1 == nullptr) wItemCss1 = m_Current;
			T* wItemCss2 = (*m_Allocator)(E2);
			if (wItemCss2 == nullptr) wItemCss2 = m_Current;
 			return (wItemCss1->Key() < wItemCss2->Key());
		}
	};

	template <class T, tFormatRef SizeTrack>
	class alignas(SkAlign) tItemCss : tClass {
	private:
		tAllocator<T, tFormatRef, SizeTrack>  m_Allocator;
		// Vector of tFormatRef
		SkVectorItem				m_Vector;

	public:
		/// @brief	Constructor
		tItemCss() : tClass() {
		}

		/// @brief	Destructor
		~tItemCss() {
			Clear();
		}
		SkVectorItem* Vector() { return(&m_Vector); }

		/// @brief	Cleat for SkAllocator
		void Clear() {
			for (tFormatRef wRef : m_Vector) {
				m_Allocator.Delete(wRef);
			}
			m_Vector.clear();
			m_Allocator.Clear();
		}
		void Init() {
			m_Allocator.Init();
		}

		/// @brief		Operator () return T*
		/// @param[in]	sIndex tFormatRef
		/// @return		T*
		inline T* operator ()(tFormatRef sIndex) { return(m_Allocator(sIndex)); }


		/// @brief		Find () elem T*
		/// @param[in]	sItemCss T*
		/// @return		SkIterator
		inline SkIteratorItem Find(T* sItemCss) {
			SkIteratorItem wWhere;
			wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), 0, tComparatorItemCss<T, SizeTrack>(&m_Allocator, sItemCss));
			return(wWhere);
		}

		inline SkIteratorItem FindEqual(T* sItemCss) {
			SkIteratorItem wWhere;
			wWhere = std::lower_bound(m_Vector.begin(), m_Vector.end(), 0, tComparatorItemCss<T, SizeTrack>(&m_Allocator, sItemCss));
			if (wWhere != m_Vector.end()) {
				if (m_Allocator(*wWhere)->Key() == sItemCss->Key()) {
					return(wWhere);
				}
			}
			return(m_Vector.end());
		}

		inline tuple<tFormatRef, T*>  Alloc(T* sItemCss) {
			T* wItemCss;
			tFormatRef wRef = 0;
			SkIteratorItem wWhere;
			wWhere = Find(sItemCss);
			if (wWhere != m_Vector.end()) {
				wRef = *wWhere;
				wItemCss= m_Allocator(wRef);
				// Add a new instance =========================================
				if (wItemCss->Key() != sItemCss->Key()) {
					tie(wRef, wItemCss) = m_Allocator.Alloc();
					(*wItemCss) = (*sItemCss);
					m_Vector.insert(wWhere, wRef);
				} else {
					// Exist ==================================================
					wItemCss = m_Allocator(wRef);
					wItemCss->Inc();
					return(make_tuple(wRef, wItemCss));
				}
			} else {
				// Insert at end 
				tie(wRef, wItemCss) = m_Allocator.Alloc();
				(*wItemCss) = (*sItemCss);
				m_Vector.insert(wWhere, wRef);
				return(make_tuple(wRef, wItemCss));
			}
			return(make_tuple(wRef, wItemCss));
		}

		void Delete(tFormatRef sItemCssRef) {
			if (sItemCssRef != 0) {
				T* wItemCss = m_Allocator(sItemCssRef);
				if (wItemCss->Count() > 1) {
					wItemCss->Dec();
					return;
				} else {
					// Delete =========================================================
					SkIteratorItem wWhere;
					wWhere = Find(wItemCss);
					if (wWhere != m_Vector.end()) {
						T* wItemCssFind = m_Allocator(*wWhere);
						// Delete instance ============================================
						if (wItemCss->Key() == wItemCssFind->Key()) {
							m_Allocator.Delete(*wWhere);
							m_Vector.erase(wWhere);
						}	else {
							tStringStream wStream;
							wStream << "On 	Delete ItemCss :" << wItemCss->Key() << " not find on Map !";
							//throw(new tExceptionInternalError(wStream.str()));
						}
				
					} else {
						tStringStream wStream;
						wStream << "On 	Delete ItemCss :" << wItemCss->Key() << " not find on Map !";
						//throw(new tExceptionInternalError(wStream.str()));
					}

					return;
				}
			}
			return;
		}

		tString Debug() {
			tStringStream wStream;
			SkIteratorItem wIterator;
			for (wIterator = m_Vector.begin(); wIterator != m_Vector.end(); wIterator++) {
				tFormatRef wRef = *wIterator;
				T* wCssItem  = m_Allocator(wRef);
				wStream << wCssItem->Key() << " Nb:" << wCssItem->Count() << " ref:" << wRef << endl;
			}
			return(wStream.str());
		}

	};

};// en of namespace

#endif // end 
