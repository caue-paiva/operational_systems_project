#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 
#include "Board.cpp"


class Game{

   private:

   Board game_board;

   int size;
   int num_of_cops;
   int money_num;

   mt19937 generator;  //atributos para gerar números aleatórios
   uniform_int_distribution<> map_elements_distrib;    
   uniform_int_distribution<> map_type_distrib;    


   int get_random_position() {
        return map_elements_distrib(generator);
   }

   void generate_game_elements(){
      if (num_of_cops >= size * size - 1){
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

      //gera dinheiro
      int money_generated = 0;
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
      game_board(Board(board_size,num_of_cops)),
      generator(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),                             
      map_elements_distrib(0, board_size - 1),
      map_type_distrib(1, 2) 
   {

   }

};