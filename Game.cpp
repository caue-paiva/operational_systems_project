#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 
#include "Board.cpp"
#include <utility>

class Game{

   private:

   //atributos para o estado inicial do tabuleiro
   Board game_board;
   int board_size;
   int num_of_cops;
   int money_num;

   //atributos para gerar números aleatórios
   mt19937 generator;  
   uniform_int_distribution<> map_elements_distrib;       

   //atributos para guardar o estado dos elementos dinâmicos do jogo
   vector<pair<int, int>> cop_positions;
   pair<int,int> robber_position;


   int get_random_position() {
        return map_elements_distrib(generator);
   }

   void generate_game_elements(){
      cout << board_size << endl;
      cout << num_of_cops << endl;
      if (num_of_cops >= (board_size * board_size) - 1){
         cout << "Error: Too many cops for the board size." << endl;
         return;
      }

      //gera policiais
      int cops_generated = 0;
      while (cops_generated < num_of_cops){ //gerar policiais até atingir o número
         int new_x = this->get_random_position(); //posições aleatórias
         int new_y = this->get_random_position();

         if (this->game_board.position_is_free(new_x,new_y)){ //posição está livre
            this->game_board.set_position(new_x,new_y,BoardState::COP);//coloca o policial
            this->cop_positions.emplace_back(pair(new_x,new_y));
            cops_generated += 1;
         }
      }

      //gera o bandido
      int robber_x = this->get_random_position(); //posição do ladrão
      int robber_y = this->get_random_position();
      while (!this->game_board.position_is_free(robber_x,robber_y)){ //enquanto a posição gerada não for de um lugar vazio
            robber_x = this->get_random_position();
            robber_y = this->get_random_position();
      }
      this->game_board.set_position(robber_x,robber_y,BoardState::ROBBER); //coloca bandido
      this->robber_position = pair(robber_x,robber_y);

      //gera dinheiro
      int money_generated = 0;
      cout << money_num << endl;
      while (money_generated < money_num){ //gerar policiais até atingir o número
         int new_x = this->get_random_position(); //posições aleatórias
         int new_y = this->get_random_position();

         if (this->game_board.position_is_free(new_x,new_y)){ //posição está livre
            this->game_board.set_position(new_x,new_y,BoardState::MONEY);// coloca o dinheiro
            money_generated += 1;
         }
      }
   }

   public:

   Game(const int board_size,const int num_of_cops): 
      board_size(board_size),
      game_board(Board(board_size)),
      num_of_cops(num_of_cops),
      generator(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),                             
      map_elements_distrib(0, board_size - 1)
   {
      this->money_num = max(board_size - (num_of_cops*4),1); //quanto maior o mapa, mais dinheiro, quanto mais policiais, menos dinheiro
      //no minimo vamos ter 1 dinheiro
      this->generate_game_elements();
   }


   void play_game(){
      this->game_board.draw_board();
   }

   void print_cops(){
      for (pair element : this->cop_positions){
         cout << "(" << element.first << ", " << element.second << ")" << endl;
      }
   }

   void print_robber(){
      cout << "(" << this->robber_position.first << ", " << this->robber_position.second << ")"  << endl;
   }

};


int main(){
   Game my_game = Game(15,2);
   my_game.play_game();
   my_game.print_cops();
   my_game.print_robber();
};