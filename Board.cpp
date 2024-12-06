#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 
#include <cstdlib>
#include "ThreeStateMutex.cpp"

/*
Classe que modela o tabuleiro, não implementa lógica do jogo (condição de vitória, game-over, movimento...) apenas guarda os elementos do tabuleiro 
como uma matriz de variaveis do enum BoardState e também guarda uma matriz de mutexes de 3 estados (ThreeStateMutex) para cada entrada na matriz de posição

OBS: Posição dos elementos é indexada por I (Linha) e J (Coluna) igual numa matriz normal.

Tem funções para mudar o estado do tabuleiro (set_position) e outras para apenas ler o estado (verificar se a posição é valida, oq tem nela...)

Também implementa uma função que escolhe randomicamente entre 3 mapas e gera o tabuleiro de acordo com o mapa escolhido (Cruz ou X).

Finalmente temos funções para printar o mapa na tela (e limpar o terminal,dando a impressão de que ocorre sempre um refresh) e para desenhar a tela de game over e de vitória
*/


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
            
            string board_string;
            for (int i = 0; i < size; i++){
                  for (int j = 0; j < size; j++){
                        board_string += " ";
                        BoardState cell = cells[i][j];
                        switch (cell) {
                           case BoardState::WALL:
                              board_string += "\033[33m"; // Cor amarela para parede
                              board_string += (cell);
                              board_string += "\033[0m"; // Reseta cor
                              break;

                           case BoardState::ROBBER:
                              board_string += "\033[31m"; // Cor vermelha para ladrão
                              board_string += (cell);
                              board_string += "\033[0m"; 
                              break;

                           case BoardState::COP:
                              board_string += "\033[34m"; // Cor azul para o policial
                              board_string += (cell);
                              board_string += "\033[0m"; 
                              break;
                           case BoardState::MONEY:
                              board_string += "\033[32m"; // Cor verde para dinheiro
                              board_string += (cell);
                              board_string += "\033[0m"; 
                              break;
                           case BoardState::EMPTY:
                           default:
                              board_string += (cell); // Sem cor para espaço vazio de movimentação
                              break;
                        }
                  }
                  board_string += "\n";
            }
            cout << board_string;
            cout << "Move the Character: Enter A W S D: " << endl;
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

   bool position_has(const int i, const int j, BoardState element) { //verifica se o elemento está presente na posição i,j
         if (!this->position_is_valid(i,j)) //posição não valida
            return false;
         if (cells[i][j] == element) //checa se existe aquele elemento naquela posição
            return true;
         return false;
   }

   BoardState get_position(const int i, const int j){
         if (!this->position_is_valid(i,j)){ //posição não valida
            cout << "função get_position acessou posição de memória invalida" << endl;
            exit(1);
         }
         return this->cells[i][j];
   }
   
   bool set_position(const int i, const int j, const BoardState state){ //seta uma posição do tabuleiro a um estado especifico
      if (!this->position_is_valid(i,j))
         return false;
      cells[i][j] = state;
      return true;
   }

   void draw_victory(){ //printa tela de vitória
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

   void draw_game_over(){ //printa tela de game over
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