//=============================================================================
// SkRoot SharedString
// Manage shared string
//=============================================================================
#include "../include/SkSharedString.hpp"
#include "../include/SkApplication.hpp"

namespace SkRoot {
		// Empty string used when a tSharedString has no pool entry
		static const tString& EmptySharedString() {
			static const tString wEmpty;
			return(wEmpty);
		}

		// SkSharedStringPool =================================================
		tSharedStringPool::tSharedStringPool() : tClass() , m_Count(0), m_Key(nullptr) {}
		tSharedStringPool::~tSharedStringPool() {}

		void tSharedStringPool::BindKey(const tString* sKey) { m_Key = sKey; }

		void tSharedStringPool::Inc() { m_Count++; }
		void tSharedStringPool::Dec() { m_Count--; }
		tBool tSharedStringPool::Empty() const { return(m_Count == 0); }

		const tString& tSharedStringPool::operator()() const {
			if (m_Key != nullptr) {
				return(*m_Key);
			}
			return(EmptySharedString());
		}

		//=========================================================================
		// Pointer on m_SharedStringContainer in SkApplication
		static tSharedStringContainer* wSharedStringContainer=nullptr;
		// there can only be one container for an application
		//=========================================================================


		// SkSharedStringContainer ================================================
		tSharedStringContainer::tSharedStringContainer() : tClass() {
			wSharedStringContainer = this;
		}
		tSharedStringContainer::~tSharedStringContainer() {
			// Remaining entries here mean tSharedString instances outlived the container
			for (auto wIterator : m_MapStringPool) {
				delete((wIterator).second);
			}
			m_MapStringPool.clear();
			// Invalidate global pointer to avoid UAF in late tSharedString dtors
			wSharedStringContainer = nullptr;
		};


		tSharedStringPool* tSharedStringContainer::Add(const tString& sString) {
			tMapStringPool::iterator wIterator = m_MapStringPool.find(sString);
			if (wIterator != m_MapStringPool.end()) {
				wIterator->second->Inc();
				return(wIterator->second);
			}
			tSharedStringPool* wSharedStringPool = new tSharedStringPool();
			wSharedStringPool->Inc();
			wIterator = m_MapStringPool.emplace(sString, wSharedStringPool).first;
			wSharedStringPool->BindKey(&(wIterator->first));
			return(wSharedStringPool);
		};

		void tSharedStringContainer::Remove(tSharedStringPool* sSharedStringPool) {
			sSharedStringPool->Dec();
			if (sSharedStringPool->Empty()) {
				tMapStringPool::iterator wIterator = m_MapStringPool.find((*sSharedStringPool)());
				if (wIterator != m_MapStringPool.end()) {
					m_MapStringPool.erase(wIterator);
				}
				delete(sSharedStringPool);
			}
		}

		tSize tSharedStringContainer::NbSharedString() const {
			return(m_MapStringPool.size());
		}


		// tSharedString =====================================================
		tSharedString::tSharedString() : tClass() {
			m_SharedStringPool = nullptr;
		}
		
		tSharedString::tSharedString(const tSharedString& sSharedString) : tClass(sSharedString) {
			m_SharedStringPool = sSharedString.m_SharedStringPool;
			if (m_SharedStringPool != nullptr) {
				m_SharedStringPool->Inc();
			}
		}

		tSharedString::tSharedString(tSharedString&& sSharedString) noexcept : tClass() {
			m_SharedStringPool = sSharedString.m_SharedStringPool;
			sSharedString.m_SharedStringPool = nullptr;
		}
			
		tSharedString::tSharedString(const tString& sString) : tClass() { 
			if (wSharedStringContainer != nullptr) {
				m_SharedStringPool = wSharedStringContainer->Add(sString);
			} else {
				m_SharedStringPool = nullptr;
			}
		}
		tSharedString::tSharedString(const tChar* sChar) : tClass() { 
			if (wSharedStringContainer != nullptr && sChar != nullptr) {
				m_SharedStringPool = wSharedStringContainer->Add(tString(sChar));
			} else {
				m_SharedStringPool = nullptr;
			}
		}

		tSharedString::~tSharedString() { 
			if ((m_SharedStringPool != nullptr) && (wSharedStringContainer != nullptr)) {
				wSharedStringContainer->Remove(m_SharedStringPool);
			}
		}

		const tString& tSharedString::Str() const noexcept {
			if (m_SharedStringPool != nullptr) {
				return((*m_SharedStringPool)());
			}
			return(EmptySharedString());
		}

		tSharedString& tSharedString::operator =(const tSharedString& sSharedString) {
            if (&sSharedString != this) {
                if ((m_SharedStringPool != nullptr) && (wSharedStringContainer != nullptr)) {
					wSharedStringContainer->Remove(m_SharedStringPool);
				}
                m_SharedStringPool = sSharedString.m_SharedStringPool;
                if (m_SharedStringPool != nullptr) {
					m_SharedStringPool->Inc();
				}
            }
			return(*this);
		}

		tSharedString& tSharedString::operator =(tSharedString&& sSharedString) noexcept {
			if (&sSharedString != this) {
				if ((m_SharedStringPool != nullptr) && (wSharedStringContainer != nullptr)) {
					wSharedStringContainer->Remove(m_SharedStringPool);
				}
				m_SharedStringPool = sSharedString.m_SharedStringPool;
				sSharedString.m_SharedStringPool = nullptr;
			}
			return(*this);
		}

		tSharedString& tSharedString::operator +=(const tString& sString) {
			if (wSharedStringContainer != nullptr) {
				if (m_SharedStringPool != nullptr) {
					tString wString = (*m_SharedStringPool)();
					wSharedStringContainer->Remove(m_SharedStringPool);
					m_SharedStringPool = wSharedStringContainer->Add(wString + sString);
				} else {
					m_SharedStringPool = wSharedStringContainer->Add(sString);
				}
			}
			return(*this);
		}
		tBool tSharedString::operator==(const tSharedString& sString) const noexcept {
			if (m_SharedStringPool == sString.m_SharedStringPool) {
				return(true);
			}
			// nullptr (unpooled empty) vs interned ""
			if (m_SharedStringPool == nullptr || sString.m_SharedStringPool == nullptr) {
				return(Str().empty() && sString.Str().empty());
			}
			return(false);
		}
		tBool tSharedString::operator!=(const tSharedString& sString) const noexcept {
			return(!(*this == sString));
		}

		tBool tSharedString::operator == (const tString& sString) const {
			return(Str() == sString);
		}
		tBool tSharedString::operator != (const tString& sString) const {
			return(Str() != sString);
		}
		tBool tSharedString::operator < (const tString& sString) const {
			return(Str() < sString);
		}
		tBool tSharedString::operator > (const tString& sString) const {
			return(Str() > sString);
		}
		tBool tSharedString::operator <= (const tString& sString) const {
			return(Str() <= sString);
		}
		tBool tSharedString::operator >= (const tString& sString) const {
			return(Str() >= sString);
		}


		const tString& tSharedString::operator()() const noexcept { return(Str()); }

		// Friend function =======================================================
		tSharedString operator+(const tSharedString& sSharedString1, const tSharedString& sSharedString2) {
			return(tSharedString(sSharedString1.Str() + sSharedString2.Str()));
		}


}; // Fin du name space ========================================================
