#pragma once

#include <condition_variable>
#include <mutex>

namespace pgsqloo {
    struct Notifier {
        std::mutex m;
        std::condition_variable cv;
        bool notified = false;
        bool finished = false;

        void Reset() {
            std::lock_guard<std::mutex> guard(m);
            notified = false;
            finished = false;
        }

        void Notify() {
            std::lock_guard<std::mutex> guard(m);
            notified = true;
            cv.notify_one();
        }

        void WaitForNotify(bool reset = true) {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [this] { return this->notified; });
            lock.unlock();
            if (reset) Reset();
        }

        void Finish() {
            std::lock_guard<std::mutex> guard(m);
            finished = true;
            cv.notify_one();
        }

        void WaitForFinish(bool reset = true) {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [this] { return this->finished; });
            lock.unlock();
            if (reset) Reset();
        }
    };
}  // namespace pgsqloo
