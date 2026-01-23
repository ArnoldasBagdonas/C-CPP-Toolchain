#pragma once

#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component for processing files with a threaded work queue.
 */
class ThreadedFileQueue
{
  public:
    /**
     * @brief Construct a threaded work queue.
     *
     * @param[in] threadCount Number of worker threads
     * @param[in] maxQueueSize Maximum queued items before producers block
     * @param[in] workItem Work item callback
     */
    ThreadedFileQueue(unsigned int threadCount, std::size_t maxQueueSize, const std::function<void(const fs::path&)>& workItem);
    /**
     * @brief Finalize and join worker threads.
     */
    ~ThreadedFileQueue();

    /**
     * @brief Enqueue a file for processing.
     *
     * @param[in] file File path to enqueue
     */
    void Enqueue(const fs::path& file);
    /**
     * @brief Signal completion and wait for workers to finish.
     */
    void Finalize();

  private:
    void WorkerLoop();

    std::size_t _maxQueueSize;
    std::function<void(const fs::path&)> _workItem;
    std::mutex _queueMutex;
    std::condition_variable _queueCv;
    std::queue<fs::path> _fileQueue;
    std::vector<std::thread> _workers;
    bool _done;
    bool _finalized;
};
