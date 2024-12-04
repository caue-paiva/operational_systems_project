#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <condition_variable>
#include <chrono> 
#include "Board.cpp"
#include <utility>
#include <cstdlib>
#include <thread> 
#include <atomic>
#include <cctype> 
#include <unistd.h>

#include <termios.h>

using namespace std;

/*
Classe que implementa toda a lógica do jogo, tem um atributo que é um objeto da classe Board, que representa o tabuleiro do jogo.

tem atributos também para guardar a posição do bandido, dos policiais e o dinheiro que tem no tabuleiro.

Ela tem métodos para gerar os elementos (Bandido, policial e dinheiro) do jogo e colocar eles no tabuleiro, para renderizar a tela de forma infinita 
(até ter game-over ou vitória).

Além disso, o input do jogador e a lógica de movimento do bandido estão implementadas nessa classe (métodos move_robber e robber_logic).


O que falta fazer:

Basicamente juntar tudo e colocar thread (Uma pro input do user, uma pra renderizar a tela e uma para cada policial) e refinar a lógica de movimento do bandido com base nos mutexes
e as threads.
*/

class Game {
private:
    // Mutex for thread-safe access to shared game state
    mutex game_mutex;
    condition_variable game_cv;

    // Atomic flag to control game state
    atomic<bool> game_running{true};

    // Game configuration
    enum class RobberAction {
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_UP,
        MOVE_DOWN,
        INVALID
    };

    // Delays in seconds for thread actions
    const int REFRESH_BOARD_DELAY = 1;
    const int USER_INPUT_DELAY = 1;
    const int COP_MOVEMENT_DELAY = 2;

    // Game board and elements
    Board game_board;
    int board_size;
    int num_of_cops;
    atomic<int> money_num;

    // Random number generation
    mt19937 generator;  
    uniform_int_distribution<> map_elements_distrib;       

    // Game element positions
    vector<pair<int, int>> cop_positions;
    pair<int,int> robber_position;

    // Threads
    vector<thread> cop_threads;
    thread render_thread;
    thread input_thread;

    // Helper methods for random positioning
    int get_random_position() {
        return map_elements_distrib(generator);
    }

    void generate_game_elements() {
        lock_guard<mutex> lock(game_mutex);

        if (num_of_cops >= (board_size * board_size) - 1) {
            cout << "Error: Too many cops for the board size." << endl;
            return;
        }

        // Generate cops
        int cops_generated = 0;
        while (cops_generated < num_of_cops) {
            int new_i = get_random_position();
            int new_j = get_random_position();

            if (game_board.position_is_free(new_i, new_j)) {
                game_board.set_position(new_i, new_j, BoardState::COP);
                cop_positions.emplace_back(new_i, new_j);
                cops_generated++;
            }
        }

        // Generate robber
        int robber_i = get_random_position();
        int robber_j = get_random_position();
        while (!game_board.position_is_free(robber_i, robber_j)) {
            robber_i = get_random_position();
            robber_j = get_random_position();
        }
        game_board.set_position(robber_i, robber_j, BoardState::ROBBER);
        robber_position = make_pair(robber_i, robber_j);

        // Generate money
        int money_generated = 0;
        while (money_generated < money_num) {
            int new_i = get_random_position();
            int new_j = get_random_position();

            if (game_board.position_is_free(new_i, new_j)) {
                game_board.set_position(new_i, new_j, BoardState::MONEY);
                money_generated++;
            }
        }
    }

    void move_cop(int cop_index) {
    while (game_running) {
        {
            unique_lock<mutex> lock(game_mutex);
            game_cv.wait_for(lock, chrono::seconds(COP_MOVEMENT_DELAY), [this] { return !game_running; });

            if (!game_running) break;

            // Cop movement logic
            pair<int, int>& cop_pos = cop_positions[cop_index];
            vector<pair<int, int>> possible_moves = {
                {cop_pos.first-1, cop_pos.second},
                {cop_pos.first+1, cop_pos.second},
                {cop_pos.first, cop_pos.second-1},
                {cop_pos.first, cop_pos.second+1}
            };

            // Manually filter valid moves
            vector<pair<int, int>> valid_moves;
            for (const auto& move : possible_moves) {
                if (game_board.position_is_valid(move.first, move.second) && 
                    !game_board.position_has(move.first, move.second, BoardState::WALL) &&
                    !game_board.position_has(move.first, move.second, BoardState::COP)) {
                    valid_moves.push_back(move);
                }
            }

            // If valid moves exist, choose randomly
            if (!valid_moves.empty()) {
                int move_index = get_random_position() % valid_moves.size();
                pair<int, int> new_pos = valid_moves[move_index];

                // Update board
                game_board.set_position(cop_pos.first, cop_pos.second, BoardState::EMPTY);
                game_board.set_position(new_pos.first, new_pos.second, BoardState::COP);
                cop_pos = new_pos;

                // Verifica se o ladrão foi pego
                if (new_pos == robber_position) {
                    game_over();
                }
            }
        }
        this_thread::sleep_for(chrono::seconds(COP_MOVEMENT_DELAY));
    }
}

void set_input_mode() {
    struct termios new_termios;

    // Obtém as configurações atuais da terminal
    tcgetattr(STDIN_FILENO, &new_termios);

    // Desativa o modo de buffering e a necessidade de pressionar Enter
    new_termios.c_lflag &= ~(ICANON | ECHO); 
    new_termios.c_cc[VMIN] = 1;  // Leitura de 1 caractere
    new_termios.c_cc[VTIME] = 0; // Sem tempo limite

    // Aplica as configurações
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void restore_input_mode() {
    struct termios new_termios;

    // Restaura as configurações originais da terminal
    tcgetattr(STDIN_FILENO, &new_termios);
    new_termios.c_lflag |= (ICANON | ECHO);  // Restaura o buffering
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

char get_char_no_enter() {
    char ch;
    set_input_mode();
    ch = getchar();  // Lê um único caractere
    restore_input_mode();
    return ch;
}

void handle_user_input() {
    while (game_running) {
        {
            unique_lock<mutex> lock(game_mutex);
            game_cv.wait_for(lock, chrono::seconds(USER_INPUT_DELAY), [this] { return !game_running; });

            if (!game_running) break;
        }

        // Pega o input do usuário
        char input = get_char_no_enter(); // Pega um único caractere
        char uppercase = toupper(input);

        {
            lock_guard<mutex> lock(game_mutex);

            // Move o ladrão com base no input do usuário
            int robber_i = robber_position.first;
            int robber_j = robber_position.second;
            int new_i = robber_i;
            int new_j = robber_j;

            switch (uppercase) {
                case 'A': new_j = robber_j - 1; break;
                case 'W': new_i = robber_i - 1; break;
                case 'S': new_i = robber_i + 1; break;
                case 'D': new_j = robber_j + 1; break;
                default:
                    cout << "Invalid Input" << endl;
                    continue;
            }

            // Valida o movimento
            if (game_board.position_is_valid(new_i, new_j) && !game_board.position_has(new_i, new_j, BoardState::WALL)) {
                robber_logic(new_i, new_j, robber_i, robber_j);
            } else {
                cout << "Invalid Position" << endl;
            }
        }

        this_thread::sleep_for(chrono::seconds(USER_INPUT_DELAY));
    }
}



    void render_game_board() {
        while (game_running) {
            {
                lock_guard<mutex> lock(game_mutex);
                game_board.draw_board();
            }
            this_thread::sleep_for(chrono::seconds(REFRESH_BOARD_DELAY));
        }
    }

    void robber_logic(const int new_i, const int new_j, const int old_i, const int old_j) {
        BoardState element = game_board.get_position(new_i, new_j);

        switch (element) {
            case BoardState::COP:
                game_over();
                break;
            case BoardState::MONEY:
                game_board.set_position(new_i, new_j, BoardState::ROBBER);
                game_board.set_position(old_i, old_j, BoardState::EMPTY);
                robber_position = make_pair(new_i, new_j);
                money_num--;
                cout << "Got money" << endl;
                break;
            case BoardState::EMPTY:
                game_board.set_position(new_i, new_j, BoardState::ROBBER);
                game_board.set_position(old_i, old_j, BoardState::EMPTY);
                robber_position = make_pair(new_i, new_j);
                break;
            default:
                break;
        }

        if (money_num == 0) {
            game_win();
        }
    }

    void game_over() {
        game_running = false;
        game_cv.notify_all();
        game_board.draw_game_over();
        cout << "Game Over, You Lost!" << endl;
        exit(0);
    }

    void game_win() {
        game_running = false;
        game_cv.notify_all();
        game_board.draw_victory();
        cout << "Game Over, You Won!!!!" << endl;
        exit(0);
    }

public:
    Game(const int board_size, const int num_of_cops) : 
        board_size(board_size),
        game_board(Board(board_size)),
        num_of_cops(num_of_cops),
        generator(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),                             
        map_elements_distrib(0, board_size - 1)
    {
        // Calculate money based on board size and number of cops
        money_num = max(board_size - (num_of_cops*6), 1);
        
        // Generate initial game elements
        generate_game_elements();
    }

    void start_game() {
        // Start rendering thread
        render_thread = thread(&Game::render_game_board, this);

        // Start input handling thread
        input_thread = thread(&Game::handle_user_input, this);

        // Start cop movement threads
        for (int i = 0; i < num_of_cops; ++i) {
            cop_threads.emplace_back(&Game::move_cop, this, i);
        }

        // Wait for threads to complete
        render_thread.join();
        input_thread.join();
        for (auto& cop_thread : cop_threads) {
            cop_thread.join();
        }
    }

    ~Game() {
        // Ensure threads are stopped
        game_running = false;
        game_cv.notify_all();
    }
};

int main() {
    Game my_game(15, 2);
    my_game.start_game();
    return 0;
}