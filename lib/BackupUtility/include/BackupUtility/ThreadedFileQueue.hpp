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
    ThreadedFileQueue(unsigned int threadCount, std::size_t maxQueueSize, const std::function<void(const fs::path&)>& workItem);
    ~ThreadedFileQueue();

    void Enqueue(const fs::path& file);
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
