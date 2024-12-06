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

    // Função para acessar como policiais
    void access_by_cops() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return state == NOT_ACCESSED; });
        state = ACCESSED_BY_COPS;
    }

    // Função para acessar como ladrão
    void access_by_robber() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return state == NOT_ACCESSED; });
        state = ACCESSED_BY_ROBBER;
    }

    // Função para liberar o acesso
    void release_access() {
        std::unique_lock<std::mutex> lock(mtx);
        state = NOT_ACCESSED;
        cv.notify_all();
    }

    // Obter o estado atual (para depuração ou lógica)
    AccessState get_state() {
        std::lock_guard<std::mutex> lock(mtx);
        return state;
    }
};