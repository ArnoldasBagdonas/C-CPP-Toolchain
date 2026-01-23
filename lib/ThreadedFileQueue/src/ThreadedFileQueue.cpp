#include "ThreadedFileQueue/ThreadedFileQueue.hpp"

ThreadedFileQueue::ThreadedFileQueue(unsigned int threadCount, std::size_t maxQueueSize, const std::function<void(const fs::path&)>& workItem)
    : _maxQueueSize(maxQueueSize), _workItem(workItem), _done(false), _finalized(false)
{
    _workers.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        _workers.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadedFileQueue::~ThreadedFileQueue()
{
    Finalize();
}

void ThreadedFileQueue::Enqueue(const fs::path& file)
{
    std::unique_lock lock(_queueMutex);
    _queueCv.wait(lock, [&]() { return _fileQueue.size() < _maxQueueSize; });
    _fileQueue.push(file);
    _queueCv.notify_all();
}

void ThreadedFileQueue::Finalize()
{
    {
        std::lock_guard lock(_queueMutex);
        if (true == _finalized)
        {
            return;
        }
        _done = true;
        _finalized = true;
    }
    _queueCv.notify_all();

    for (auto& worker : _workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

/**
 * @brief Worker thread loop for processing queued files.
 */
void ThreadedFileQueue::WorkerLoop()
{
    while (true)
    {
        fs::path file;
        {
            std::unique_lock lock(_queueMutex);
            _queueCv.wait(lock, [&]() { return _done || false == _fileQueue.empty(); });
            if (true == _fileQueue.empty() && true == _done)
            {
                return;
            }
            if (true == _fileQueue.empty())
            {
                continue;
            }
            file = std::move(_fileQueue.front());
            _fileQueue.pop();
            _queueCv.notify_all();
        }
        _workItem(file);
    }
}
