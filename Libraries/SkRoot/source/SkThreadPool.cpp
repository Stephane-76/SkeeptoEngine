//=============================================================================
// SkRoot ThreadPool (Generic)
//=============================================================================

#include "../include/SkThreadPool.hpp"
#include <stdexcept>

namespace SkRoot {

    tThreadPool* tThreadPool::s_instance = nullptr;
#ifndef __EMSCRIPTEN__
    std::mutex tThreadPool::s_instanceMutex;
#endif
    std::atomic<bool> tThreadPool::s_instanceCreated(false);

    tThreadPool::tThreadPool(size_t sNumThreads, size_t sMaxQueueSize) 
#ifndef __EMSCRIPTEN__
        : m_maxQueueSize(sMaxQueueSize)
        , m_stopping(false)
#else
        : m_stopping(false)
#endif
    {
#ifndef __EMSCRIPTEN__
        if (sNumThreads == 0) sNumThreads = 1;
        m_workers.reserve(sNumThreads);
        for (size_t i = 0; i < sNumThreads; ++i) {
            m_workers.emplace_back(&tThreadPool::WorkerLoop, this);
        }
#endif
    }
    
#ifndef __EMSCRIPTEN__
    void tThreadPool::WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() { return m_stopping.load() || !m_tasks.empty(); });
                if (m_stopping.load() && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop();
                
                // Notify that queue has space if max size is set
                if (m_maxQueueSize > 0) {
                    m_cvFull.notify_one();
                }
            }
            
            // Execute task with exception handling
            try {
                task();
            } catch (...) {
                // Swallow exceptions to prevent worker thread from terminating
                // Exceptions are still accessible via the future returned by Submit()
            }
        }
    }
#endif

    tThreadPool::~tThreadPool() {
        Shutdown();
    }
    
    void tThreadPool::Shutdown() {
        if (m_stopping.load()) return;  // Already shutting down
        m_stopping = true;
        
#ifndef __EMSCRIPTEN__
        {
            std::unique_lock<std::mutex> lock(m_mutex);
        }
        m_cv.notify_all();
        m_cvFull.notify_all();
        
        for (auto& w : m_workers) {
            if (w.joinable()) w.join();
        }
#endif
    }
    
    size_t tThreadPool::PendingTasks() const {
#ifdef __EMSCRIPTEN__
        return 0;  // No queue in WASM mode
#else
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_tasks.size();
#endif
    }

    tThreadPool* tThreadPool::Instance() {
        // Double-checked locking pattern for thread-safe singleton
        if (!s_instanceCreated.load(std::memory_order_acquire)) {
#ifdef __EMSCRIPTEN__
            // In WASM, no need for mutex (single-threaded)
            if (!s_instanceCreated.load(std::memory_order_relaxed)) {
                s_instance = new tThreadPool();
                s_instanceCreated.store(true, std::memory_order_release);
            }
#else
            std::lock_guard<std::mutex> lock(s_instanceMutex);
            if (!s_instanceCreated.load(std::memory_order_relaxed)) {
                s_instance = new tThreadPool();
                s_instanceCreated.store(true, std::memory_order_release);
            }
#endif
        }
        return s_instance;
    }
    
    void tThreadPool::ShutdownInstance() {
#ifdef __EMSCRIPTEN__
        // In WASM, no need for mutex (single-threaded)
#else
        std::lock_guard<std::mutex> lock(s_instanceMutex);
#endif
        
        if (s_instance != nullptr) {
            s_instance->Shutdown();
            delete s_instance;
            s_instance = nullptr;
            // Reset flag to allow recreation if needed
            s_instanceCreated.store(false, std::memory_order_release);
        }
    }
}


