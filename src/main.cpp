#include "../include/SearchDirectory.h"

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <tuple>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    template <class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace_back([f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)] {
                std::apply(f, args);
            });
        }
        condition.notify_one();
    }

    void stopExecution() {
        stop = true;
        condition.notify_all();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

bool recursion(ThreadPool& pool, std::string filePath, std::string fileName) {
    std::vector<std::string> filePaths = findFileInDirectory(filePath, fileName);
    if (filePaths.size() != 0) {
        if (filePaths[1] == fileName) {
            std::cout << filePaths[0] + filePaths[2] << std::endl;
            pool.stopExecution();
            return true;
        } else {
            for (int i = 1; i < filePaths.size(); i++) {
                if (recursion(pool, filePaths[0] + filePaths[i], fileName))
                    return true;
            }
        }
    }
    return false;
}

int main() {
    std::string fileName = "";
    std::cout << "Enter Filename: ";
    std::cin >> fileName;

    std::cout << "Looking for file with name: " << fileName << std::endl;

    int numThreads = std::thread::hardware_concurrency();
    ThreadPool pool(numThreads > 8 ? 8 : numThreads);

    if (!recursion(pool, "C:\\", fileName)) {
        std::cout << "Nothing found" << '\n';
    }


    system ("pause");

    return 0;
}
