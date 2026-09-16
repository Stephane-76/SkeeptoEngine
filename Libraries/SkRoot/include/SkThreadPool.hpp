//=============================================================================
// SkRoot ThreadPool (Generic)
//=============================================================================
#ifndef SkThreadPool_hpp
#define SkThreadPool_hpp

#include "SkApplication.hpp"
#include <vector>
#include <future>
#include <functional>
#include <atomic>
#include <stdexcept>
#ifndef __EMSCRIPTEN__
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#endif

namespace SkRoot {

    class tThreadPool {
    private:
#ifndef __EMSCRIPTEN__
        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        mutable std::mutex m_mutex;  // mutable for const methods
        std::condition_variable m_cv;
        std::condition_variable m_cvFull;
        size_t m_maxQueueSize;  // 0 means unlimited
        // Worker thread function with exception handling
        void WorkerLoop();
#endif
        std::atomic<bool> m_stopping;

    public:
#ifdef __EMSCRIPTEN__
        explicit tThreadPool(size_t sNumThreads = 1, size_t sMaxQueueSize = 0);
#else
        explicit tThreadPool(size_t sNumThreads = std::thread::hardware_concurrency(), size_t sMaxQueueSize = 0);
#endif
        ~tThreadPool();

        tThreadPool(const tThreadPool&) = delete;
        tThreadPool& operator=(const tThreadPool&) = delete;

        // Submit a task to the thread pool
        // Returns a future that will contain the result
        // Throws std::runtime_error if pool is stopped or queue is full
        template<typename F, typename... Args>
        auto Submit(F&& sFunc, Args&&... sArgs) -> std::future<typename std::invoke_result_t<F, Args...>> {
            using ReturnType = typename std::invoke_result_t<F, Args...>;

#ifdef __EMSCRIPTEN__
            // In WASM (without pthreads), execute immediately on main thread
            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(sFunc), std::forward<Args>(sArgs)...)
            );
            std::future<ReturnType> res = task->get_future();
            (*task)();
            return res;
#else
            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(sFunc), std::forward<Args>(sArgs)...)
            );

            std::future<ReturnType> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                
                // Wait if queue is full (if max size is set)
                if (m_maxQueueSize > 0) {
                    m_cvFull.wait(lock, [this]() { 
                        return m_stopping.load() || m_tasks.size() < m_maxQueueSize; 
                    });
                }
                
                if (m_stopping.load()) {
                    throw std::runtime_error("tThreadPool stopped");
                }
                
                m_tasks.emplace([task]() { (*task)(); });
            }
            m_cv.notify_one();
            return res;
#endif
        }

        // Stop accepting new tasks and wait for all tasks to complete
        void Shutdown();
        
        // Check if pool is stopped
        bool IsStopped() const { return m_stopping.load(); }
        
        // Get number of pending tasks
        size_t PendingTasks() const;
        
        // Get number of worker threads
        size_t WorkerCount() const {
#ifdef __EMSCRIPTEN__
            return 1;  // Single-threaded in WASM
#else
            return m_workers.size();
#endif
        }
        
        // Singleton instance (thread-safe)
        static tThreadPool* Instance();
        
        // Shutdown and destroy singleton instance
        static void ShutdownInstance();
        
    private:
        static tThreadPool* s_instance;
#ifndef __EMSCRIPTEN__
        static std::mutex s_instanceMutex;
#endif
        static std::atomic<bool> s_instanceCreated;
    };
}

#endif


