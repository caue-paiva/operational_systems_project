#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 
#include "Board.cpp"
#include <utility>
#include <cstdlib>
#include <thread> 
#include <cctype> 


using namespace std;

class Game{ //classe que lida com a lógica do jogo e as threads

   private:

   enum RobberAction {

   };

   //delays em segundos para as ações de cada thread
   const int refresh_board_delay = 1;
   const int user_input_delay = 1;
   const int cop_movement_delay = 2;


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
      if (num_of_cops >= (board_size * board_size) - 1){
         cout << "Error: Too many cops for the board size." << endl;
         return;
      }

      //gera policiais
      int cops_generated = 0;
      while (cops_generated < num_of_cops){ //gerar policiais até atingir o número
         int new_i = this->get_random_position(); //posições aleatórias
         int new_j = this->get_random_position();

         if (this->game_board.position_is_free(new_i,new_j)){ //posição está livre
            this->game_board.set_position(new_i,new_j,BoardState::COP);//coloca o policial
            this->cop_positions.emplace_back(pair(new_i,new_j));
            cops_generated += 1;
         }
      }

      //gera o bandido
      int robber_i = this->get_random_position(); //posição do ladrão
      int robber_j = this->get_random_position();
      while (!this->game_board.position_is_free(robber_i,robber_j)){ //enquanto a posição gerada não for de um lugar vazio
            robber_i = this->get_random_position();
            robber_j = this->get_random_position();
      }
      this->game_board.set_position(robber_i,robber_j,BoardState::ROBBER); //coloca bandido
      this->robber_position = pair(robber_i,robber_j);

      //gera dinheiro
      int money_generated = 0;
      while (money_generated < money_num){ //gerar policiais até atingir o número
         int new_i = this->get_random_position(); //posições aleatórias
         int new_j = this->get_random_position();

         if (this->game_board.position_is_free(new_i,new_j)){ //posição está livre
            this->game_board.set_position(new_i,new_j,BoardState::MONEY);// coloca o dinheiro
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

   void move_robber(){ //função que pega input do usuário e que move o bandido

     //pegar input do usuário
      cout << "Mova o Personagem: Digite A W S D: " << endl;
      char input;
      cin >> input;
      char uppercase = toupper(input);

      int robber_i = this->robber_position.first; //posição atual do bandido
      int robber_j = this->robber_position.second;

      int new_i = robber_i; //começa com posição atual
      int new_j = robber_j;

      switch (uppercase) //calcula nova posi
      {
         case 'A': // move para esq
            new_j = robber_j - 1;
            break;
         case 'W': // move para cima
            new_i = robber_i - 1;
            break;
         case 'S': // move para baixo
            new_i = robber_i + 1;
            break;
         case 'D': // move para dir
            new_j = robber_j + 1;
            break;
         default:
            cout << "Input inválido" << endl;
            break;
      }

      //posição deve ser válida e não deve ser parede
      if (this->game_board.position_is_valid(new_i, new_j) &&  !this->game_board.position_has(new_i,new_j,BoardState::WALL)) {
         this->robber_logic(new_i,new_j,robber_i,robber_j); //lógica de movimento do bandido
      } else {
         cout << "Posição Não valida" << endl;
      }
   }

   void robber_logic(const int new_i, const int new_j, const int old_i, const int old_j){ //lógica para mover bandido caso a nova posição não seja invalida ou parede
      BoardState element = this->game_board.get_position(new_i, new_j); //elemento presente na nova posição

      switch (element)
      {
         case BoardState::COP:
            this->game_over(); //perdeu mané
            break;
         case BoardState::MONEY:
            this->game_board.set_position(new_i, new_j, BoardState::ROBBER);
            this->game_board.set_position(old_i, old_j, BoardState::EMPTY); //posição antiga fica vazia
            this->robber_position = pair(new_i,new_j);
            this->money_num--; //decrementa número de dinheiro
            break;
         case BoardState::EMPTY: //move para uma nova posição
            this->game_board.set_position(new_i, new_j, BoardState::ROBBER); //atualiza posição do bandido
            this->game_board.set_position(old_i, old_j, BoardState::EMPTY);  //posição antiga fica vazia
            this->robber_position = pair(new_i,new_j);
            break;
         default:
            break;
      }

      if (this->money_num == 0){
         this->game_win(); //bandido roubou todo dinheiro, jogador ganhou
      }

   }

   void render_game_board(){ //função para thread renderizar o jogo
      while (true) { //loop infinito
         this->game_board.draw_board(); //desenha tabuleiro
         this_thread::sleep_for(chrono::seconds(this->refresh_board_delay));
      }
   }

   void print_cops(){
      for (pair element : this->cop_positions){
         cout << "(" << element.first << ", " << element.second << ")" << endl;
      }
   }

   void print_robber(){
      cout << "(" << this->robber_position.first << ", " << this->robber_position.second << ")"  << endl;
   }

   void game_over(){ //jogador perdeu
      //matar as threads aqui
      this->game_board.draw_game_over();
      cout << "Jogo Acabou, você perdeu!" << endl;
      exit(0);
   }

   void game_win(){ //jogador ganhou
      //mata as threads aqui
      this->game_board.draw_victory();
      cout << "Jogo Acabou, você Ganhou!!!!" << endl;
      exit(0);
   }

};


int main(){
   Game my_game = Game(15,2);
   
   for (int i = 0 ; i < 10 ; i++){
         my_game.play_game();
         my_game.move_robber();
         this_thread::sleep_for(chrono::seconds(2));         
   }

};