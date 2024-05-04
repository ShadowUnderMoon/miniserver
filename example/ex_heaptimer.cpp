#include <spdlog/spdlog.h>
#include <heaptimer.h>
#include <unistd.h>
using namespace std::chrono_literals;


int main() {
    HeapTimer heap_timer;
    heap_timer.Add(1, 10s, [&] { heap_timer.Del(1);}) ;
    heap_timer.Add(2, 10s, [&] { heap_timer.Del(2);});

    heap_timer.Add(3, 10s, [&] { heap_timer.Del(3);});
    sleep(5);
    heap_timer.Extend(1, 10s);
    heap_timer.Del(1);
    heap_timer.GetNextTick();
}