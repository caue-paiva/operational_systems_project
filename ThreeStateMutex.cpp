#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>

/*
Mutex com 3 estados

1) Livre

2) Acessado pelo policial

3) Acessado pelo bandido

Cada bloco do tabuleiro tem um mutex dessa associado, para evitar race conditions e controlar a lógica de movimento e do jogo
*/

enum AccessState {
    NOT_ACCESSED,
    ACCESSED_BY_COPS,
    ACCESSED_BY_ROBBER
};

class ThreeStateMutex {
private:
    std::mutex mtx;
    std::condition_variable cv;
    AccessState state;

public:
    ThreeStateMutex() : state(NOT_ACCESSED) {}

    // Function to access as cops
    void access_by_cops() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return state == NOT_ACCESSED; });
        state = ACCESSED_BY_COPS;
    }

    // Function to access as robber
    void access_by_robber() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return state == NOT_ACCESSED; });
        state = ACCESSED_BY_ROBBER;
    }

    // Function to release access
    void release_access() {
        std::unique_lock<std::mutex> lock(mtx);
        state = NOT_ACCESSED;
        cv.notify_all();
    }

    // Get current state (for debugging or logic)
    AccessState get_state() {
        std::lock_guard<std::mutex> lock(mtx);
        return state;
    }
};