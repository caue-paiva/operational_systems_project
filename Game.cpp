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
*/

class Game {
    private:
        // Mutex para acesso thread-safe ao estado de jogo compartilhado
        mutex game_mutex;
        condition_variable game_cv;

        // Bandeira atômica para controlar o estado do jogo
        atomic<bool> game_running{true};

        // Configuração do jogo
        enum class RobberAction {
            MOVE_LEFT,
            MOVE_RIGHT,
            MOVE_UP,
            MOVE_DOWN,
            INVALID
        };

        // Atrasos em segundos para ações de thread
        const int REFRESH_BOARD_DELAY = 100;
        const int USER_INPUT_DELAY = 100;
        const int COP_MOVEMENT_DELAY = 1000;

        // Tabuleiro e elementos do jogo
        Board game_board;
        int board_size;
        int num_of_cops;
        atomic<int> money_num;

        // Geração de números aleatórios
        mt19937 generator;  
        uniform_int_distribution<> map_elements_distrib;       

        // Posições dos elementos do jogo
        vector<pair<int, int>> cop_positions;
        pair<int,int> robber_position;

        // Threads
        vector<thread> cop_threads;
        thread render_thread;
        thread input_thread;

        // Métodos auxiliares para posicionamento aleatório
        int get_random_position() {
            return map_elements_distrib(generator);
        }

        void generate_game_elements() {
            lock_guard<mutex> lock(game_mutex);

            if (num_of_cops >= (board_size * board_size) - 1) {
                cout << "Error: Too many cops for the board size." << endl;
                return;
            }

            // Gere policiais primeiro
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

            // Gerar ladrão com verificação de distância segura
            bool valid_robber_position = false;
            int robber_i, robber_j;
            const int SAFE_DISTANCE = 3; // Distância mínima segura dos policiais

            while (!valid_robber_position) {
                robber_i = get_random_position();
                robber_j = get_random_position();
                
                if (!game_board.position_is_free(robber_i, robber_j)) {
                    continue;
                }

                // Verifica a distância de todos os policiais
                bool is_safe = true;
                for (const auto& cop_pos : cop_positions) {
                    int distance = abs(cop_pos.first - robber_i) + 
                                abs(cop_pos.second - robber_j);
                    if (distance < SAFE_DISTANCE) {
                        is_safe = false;
                        break;
                    }
                }

                if (is_safe) {
                    valid_robber_position = true;
                }
            }

            game_board.set_position(robber_i, robber_j, BoardState::ROBBER);
            robber_position = make_pair(robber_i, robber_j);

            // Geração de dinheiro
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

    bool is_adjacent_to_robber(const pair<int, int>& cop_pos) {
        // Verifica todas as posições adjacentes (incluindo diagonais)
        vector<pair<int, int>> adjacent_positions = {
            {cop_pos.first-1, cop_pos.second},   // cima
            {cop_pos.first+1, cop_pos.second},   // baixo
            {cop_pos.first, cop_pos.second-1},   // esquerda
            {cop_pos.first, cop_pos.second+1},   // direita
            {cop_pos.first-1, cop_pos.second-1}, // diagonal superior esquerda
            {cop_pos.first-1, cop_pos.second+1}, // diagonal superior direita
            {cop_pos.first+1, cop_pos.second-1}, // diagonal inferior esquerda
            {cop_pos.first+1, cop_pos.second+1}  // diagonal inferior direita
        };

        for (const auto& pos : adjacent_positions) {
            if (game_board.position_is_valid(pos.first, pos.second) && 
                pos == robber_position) {
                return true;
            }
        }
        return false;
    }

    void move_cop(int cop_index) {
        while (game_running) {
            {
                unique_lock<mutex> lock(game_mutex);
                game_cv.wait_for(lock, chrono::microseconds(COP_MOVEMENT_DELAY), [this] { return !game_running; });

                if (!game_running) break;

                pair<int, int>& cop_pos = cop_positions[cop_index];
                
                // Verifica se o ladrão está adjacente
                if (is_adjacent_to_robber(cop_pos)) {
                    game_over();
                    break;
                }

                // Calcula a distância até o ladrão
                int distance_to_robber = abs(cop_pos.first - robber_position.first) + 
                                    abs(cop_pos.second - robber_position.second);

                // Se o ladrão estiver a uma distância menor que 5 blocos, persegue-o
                if (distance_to_robber < 5) {
                    vector<pair<int, int>> possible_moves = {
                        {cop_pos.first-1, cop_pos.second},   // cima
                        {cop_pos.first+1, cop_pos.second},   // baixo
                        {cop_pos.first, cop_pos.second-1},   // esquerda
                        {cop_pos.first, cop_pos.second+1}    // direita
                    };

                    // Encontra o melhor movimento (que minimiza a distância até o ladrão)
                    pair<int, int> best_move = cop_pos;
                    int min_distance = distance_to_robber;

                    for (const auto& move : possible_moves) {
                        if (game_board.position_is_valid(move.first, move.second) && 
                            !game_board.position_has(move.first, move.second, BoardState::WALL) &&
                            !game_board.position_has(move.first, move.second, BoardState::COP) &&
                            !game_board.position_has(move.first, move.second, BoardState::MONEY)) {  
                            
                            int new_distance = abs(move.first - robber_position.first) + 
                                            abs(move.second - robber_position.second);
                            
                            if (new_distance < min_distance) {
                                min_distance = new_distance;
                                best_move = move;
                            }
                        }
                    }

                    // Move para a melhor posição se encontrou um movimento válido
                    if (best_move != cop_pos) {
                        // Remove o policial da posição atual
                        game_board.set_position(cop_pos.first, cop_pos.second, 
                            game_board.get_position(cop_pos.first, cop_pos.second) == BoardState::COP 
                            ? BoardState::EMPTY 
                            : game_board.get_position(cop_pos.first, cop_pos.second)
                        );

                        // Move o policial para a nova posição
                        game_board.set_position(best_move.first, best_move.second, BoardState::COP);
                        cop_pos = best_move;
                    }
                } else {
                    // Movimento aleatório
                    vector<pair<int, int>> possible_moves = {
                        {cop_pos.first-1, cop_pos.second},
                        {cop_pos.first+1, cop_pos.second},
                        {cop_pos.first, cop_pos.second-1},
                        {cop_pos.first, cop_pos.second+1}
                    };

                    vector<pair<int, int>> valid_moves;
                    for (const auto& move : possible_moves) {
                        if (game_board.position_is_valid(move.first, move.second) && 
                            !game_board.position_has(move.first, move.second, BoardState::WALL) &&
                            !game_board.position_has(move.first, move.second, BoardState::COP) &&
                            !game_board.position_has(move.first, move.second, BoardState::MONEY)) {  
                            valid_moves.push_back(move);
                        }
                    }

                    if (!valid_moves.empty()) {
                        int move_index = get_random_position() % valid_moves.size();
                        pair<int, int> new_pos = valid_moves[move_index];

                        // Remove o policial da posição atual
                        game_board.set_position(cop_pos.first, cop_pos.second, 
                            game_board.get_position(cop_pos.first, cop_pos.second) == BoardState::COP 
                            ? BoardState::EMPTY 
                            : game_board.get_position(cop_pos.first, cop_pos.second)
                        );

                        // Move o policial para a nova posição
                        game_board.set_position(new_pos.first, new_pos.second, BoardState::COP);
                        cop_pos = new_pos;
                    }
                }
            }
            this_thread::sleep_for(chrono::milliseconds(COP_MOVEMENT_DELAY));
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
        ch = getchar(); 
        restore_input_mode();
        return ch;
    }

    void handle_user_input() {
        while (game_running) {
            {
                unique_lock<mutex> lock(game_mutex);
                if (!game_running) break;
            }

            // Pega o input do usuário
            char input = get_char_no_enter();
            char uppercase = toupper(input);

            {
                lock_guard<mutex> lock(game_mutex);

                // Move o ladrão com base no input do usuário
                int robber_i = robber_position.first;
                int robber_j = robber_position.second;
                int new_i = robber_i;
                int new_j = robber_j;

                // Botões de movimentação

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
        }
    }

    

    void render_game_board() {
        while (game_running) {
            {
                lock_guard<mutex> lock(game_mutex);
                game_board.draw_board();
            }
            this_thread::sleep_for(chrono::milliseconds(REFRESH_BOARD_DELAY));
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
        restore_input_mode();
        game_running = false;
        game_cv.notify_all();
        game_board.draw_game_over();
        cout << "Game Over, You Lost!" << endl;
        exit(0);
    }

    void game_win() {
        restore_input_mode();
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
            // Calcular dinheiro com base no tamanho do conselho e no número de policiais
            money_num = max(board_size - (num_of_cops*2), 1);
            
            // Gerar elementos iniciais do jogo
            generate_game_elements();
        }

    void start_game() {
        // Iniciar renderização do thread
        render_thread = thread(&Game::render_game_board, this);

        // Iniciar thread de manipulação de entrada
        input_thread = thread(&Game::handle_user_input, this);

        // Aguarde 2 segundos antes de iniciar os tópicos de movimentação do policial
        this_thread::sleep_for(chrono::seconds(2));

        // Iniciar tópicos de movimento policial
        for (int i = 0; i < num_of_cops; ++i) {
            cop_threads.emplace_back(&Game::move_cop, this, i);
        }

        // Aguarde a conclusão dos threads
        render_thread.join();
        input_thread.join();
        for (auto& cop_thread : cop_threads) {
            cop_thread.join();
        }
    }

    ~Game() {
        // Garantir que os threads sejam interrompidos
        game_running = false;
        game_cv.notify_all();
        restore_input_mode();
    }
};

int main() {
    Game my_game(15, 5);
    my_game.start_game();
    return 0;
}