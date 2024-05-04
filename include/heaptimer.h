#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

using TimeOutCallBack = std::function<void()>;
using Clock = std::chrono::system_clock;
using TimeStamp = Clock::time_point;

struct TimerNode {
    int id;
    TimeStamp expire_time;
    TimeOutCallBack callback;
    bool operator<(const TimerNode& rhs) const {
        return expire_time < rhs.expire_time;
    }
};


class HeapTimer {
public:
    HeapTimer() = default;
    void Extend(int id, std::chrono::milliseconds timeout) {
        heap_[ref_[id]].expire_time = Clock::now() + timeout;
        ShiftDown(ref_[id]);
    }

    void Add(int id, std::chrono::milliseconds timeout, const TimeOutCallBack& callback) {
        heap_.emplace_back(id, Clock::now() + timeout, callback);
        ref_[id] = heap_.size() - 1;
    }

    int GetNextTick() {
        Tick();
        int res = -1;
        if (!heap_.empty()) {
            res = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(heap_.front().expire_time - Clock::now()).count());
            if (res < 0) {
                res = 0;
            }
        }
        return res;
    }
    // when receives close event, remove corresponding entry from HeapTimer
    void Del(int id) {
        size_t index = ref_[id];
        DelByIndex(index);
        ref_.erase(id);
    }

    HeapTimer(const HeapTimer&) = delete;
    HeapTimer& operator=(const HeapTimer&) = delete;
    HeapTimer(HeapTimer&&) = delete;
    HeapTimer& operator=(HeapTimer&&) = delete;
private:
    void DelByIndex(size_t index) {
        if (index < heap_.size() - 1) {
            SwapNode(index, heap_.size() - 1);
            ref_.erase(heap_.back().id);
            heap_.pop_back();
            ShiftDown(index);
        } else {
            ref_.erase(heap_.back().id);
            heap_.pop_back();
        }
    }

    void Pop() {
        DelByIndex(0);
    }

    void Tick() {
        while (!heap_.empty()) {
            auto& node = heap_.front();
            if (node.expire_time > Clock::now()) {
                break;
            }
            node.callback();
            // call back function will delete corresponding entry, don't call Pop() again
            // Pop();
        }
    }
    // index starts from 0, if child's index is i, then parent's index is (i - 1) / 2
    // instead, index start from 1, if child's index is i, then parent's index is i / 2
    // because (i - 1) / 2 , if i == 0, then underflow/overflow
    void ShiftUp(size_t index) {
        size_t x = index + 1;
        while (x > 1 && heap_[x] < heap_[x / 2]) {
            SwapNode(x -1 , x / 2 - 1);
            x  = x / 2;
        }
    }
    // index starts from 0, if parent's index is j, then children's indices are (2i + 1), (2i + 2)
    // instead index starts from 1, is parent's index is j, then children's indices are (2i), (2i + 1)
    void ShiftDown(size_t index) {
        size_t x = index + 1;
        size_t n = heap_.size();
        while (x * 2 <= n) {
            size_t t = x * 2;
            if (t + 1 <= n && heap_[t] < heap_[t - 1]) {
                t++;
            }
            if (!(heap_[t - 1] < heap_[x - 1])) {
                break;
            }
            SwapNode(x - 1, t - 1);
            x = t;
        }
    }

    void SwapNode(size_t i, size_t j) {
        if (i == j) {
            return;
        }
        std::swap(heap_[i], heap_[j]);
        ref_[heap_[i].id] = i;
        ref_[heap_[j].id] = j;
    }
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};
