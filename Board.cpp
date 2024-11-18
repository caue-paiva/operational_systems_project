#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 
#include <cstdlib>
#include "ThreeStateMutex.cpp"


using namespace std;

enum BoardState { //enum para os estados/elementos possíveis do tabuleiro/mapa
   EMPTY = ' ',
   WALL = 'o',
   ROBBER = '@',
   COP = '#',
   MONEY = '$'
};


class Board { //classe que lida com as posições do tabuleiro, os mutexes dela e em printar as coisas

   private:
      vector<vector<BoardState>> cells; //vetor de células do mapa
      int size;
      mt19937 generator;  //atributos para gerar números aleatórios
      uniform_int_distribution<> map_type_distrib; //gerar o tipo do mapa


   void generate_walls(){ //gera o mapa do jogo, escolhendo entre 3 possibilidades
      //vamos ter  3 "mapas" diferentes e ele será escolhido de forma aletória. Não existe geração automática, os mapas são 
      //hardcoded
      int map_choice = map_type_distrib(generator);
      generate_border_walls();
        switch (map_choice) {
            case 1:
                generate_cross_with_openings();
                break;
            case 2:
                generate_x_pattern();
                break;
            default:
                cout << "Essa opção para gerar mapa não está disponível";
                break;
        }
   }

   void generate_border_walls() { //mapa 1
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (i == 0 || i == size - 1 || j == 0 || j == size - 1) {
                    cells[i][j] = WALL; // Set border cells as walls
                }
            }
        }
    }

   void generate_cross_with_openings() {
      int mid_start = size / 2 - 1;  // Start of the 3x3 center
      int mid_end = size / 2 + 1;    // End of the 3x3 center

      for (int i = 2; i < size - 2; i++) { // Start from index 2 and stop before size-2 for 2-cell edge spacing
         if (i < mid_start || i > mid_end) { // Skip the 3x3 opening in the center
               cells[i][mid_start] = WALL;   // Vertical line of the cross
               cells[i][mid_end] = WALL;
               cells[mid_start][i] = WALL;   // Horizontal line of the cross
               cells[mid_end][i] = WALL;
         }
      }
   }

   void generate_x_pattern() { //gera mapa com um X
        int mid_start = size / 2 - 1;  
        int mid_end = size / 2;      

        for (int i = 2; i < size - 2; i++) { 
            if (i < mid_start || i > mid_end) {
                cells[i][i] = WALL;                  
                cells[i][size - 1 - i] = BoardState::WALL; //coloca parede nesse lugar
            }
        }
    }

   public:

   vector<vector<mutex>> mutexes; //mutex para cada célula do mapa, vai ser público porque deve ser acessados por diversas threads
   //em um escopo amplo

   Board(const int size) //construtor da classe
        : size(size), 
          cells(size, vector<BoardState>(size, BoardState::EMPTY)), 
          generator(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),                             
          map_type_distrib(1, 2)                  
    {

      if (size < 15){
         cerr << "Erro: Tamanho mínimo para gerar o mapa é 15" <<endl;
         exit(0);

      }
      this->generate_walls(); //gera paredes do tabuleiro
    }

   void draw_board(){
            cout << "\033[2J\033[3J\033[H"; //limpa a tela e move cursor para cima.
            /*for (int i = 0; i < size + 5; i++)
            {
               cout << endl;
            }*/
            
            string board_string;
            for (int i = 0; i < size; i++){
                  for (int j = 0; j < size; j++){
                        board_string += " ";
                        BoardState cell = cells[i][j];
                        switch (cell) {
                           case BoardState::WALL:
                              board_string += "\033[33m"; // Yellow color for WALL
                              board_string += (cell);
                              board_string += "\033[0m"; // Reset color
                              break;

                           case BoardState::ROBBER:
                              board_string += "\033[31m"; // Red color for ROBBER
                              board_string += (cell);
                              board_string += "\033[0m"; // Reset color
                              break;

                           case BoardState::COP:
                              board_string += "\033[34m"; // Blue color for COP
                              board_string += (cell);
                              board_string += "\033[0m"; // Reset color
                              break;
                           case BoardState::MONEY:
                              board_string += "\033[32m"; // Blue color for COP
                              board_string += (cell);
                              board_string += "\033[0m"; // Reset color
                              break;
                           case BoardState::EMPTY:
                           default:
                              board_string += (cell); // No color for EMPTY
                              break;
                        }
                  }
                  board_string += "\n";
            }
            cout << board_string;
      }

   bool position_is_valid(const int i, const int j){
      if (i >= size || j >= size){
         return false;
      }
      return true;
   }

   bool position_is_free(const int i, const int j){ //diz se uma posição do tabuleiro é vazia
         if (!this->position_is_valid(i,j)) //posição não valida
            return false;
         if (cells[i][j] == BoardState::EMPTY)
            return true;
         return false;
      }
   
   bool set_position(const int i, const int j, const BoardState state){ //seta uma posição do tabuleiro a um estado especifico
      if (!this->position_is_valid(i,j))
         return false;
      cells[i][j] = state;
      return true;
   }

   void draw_victory(){
    std::string green = "\033[32m"; //  cor verde
    std::string reset = "\033[0m";   // Reset color

    std::cout << green;
    std::cout << R"(
 ██    ██ ██ ████████  ██████  ██████  ██  █████   ██
 ██    ██ ██    ██    ██    ██ ██   ██ ██ ██   ██  ██
 ██    ██ ██    ██    ██    ██ █████   ██ ███████  ██
  ██  ██  ██    ██    ██    ██ ██  ██  ██ ██   ██ 
   ████   ██    ██     ██████  ██   ██ ██ ██   ██  ██
    )" << reset << std::endl;
   }

   void draw_game_over(){
      string red = "\033[31m"; // cor vermelha
      string reset = "\033[0m";   //reseta a cor

      cout << red;
      cout << R"(
     ██████   █████  ███    ███  ███████     ██████  ██   ██ ███████ ██████  
   ██         ██   ██ ████  ████ ██         ██   ██  ██  ██  ██      ██   ██ 
   ██   ███   ███████ ██ ████ ██ █████      ██   ██   ████   █████   ██████  
   ██    ██   ██   ██ ██  ██  ██ ██         ██   ██    ██    ██      ██   ██ 
     ██████   ██   ██ ██      ██ ███████     ██████    ██    ███████ ██   ██                                                                      
    )" << reset << endl;
   }

};