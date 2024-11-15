#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>
#include <chrono> 


using namespace std;

enum BoardState {
   EMPTY = ' ',
   WALL = 'o',
   ROBBER = '@',
   COP = '#'
};


class Board {

   private:
      vector<vector<BoardState>> cells; //vetor de células do mapa
      vector<vector<mutex>> mutexes; //mutex para cada célula do mapa
      
      int size;
      int num_of_cops;

      mt19937 generator;  //atributos para gerar números aleatórios
      uniform_int_distribution<> map_elements_distrib;    
      uniform_int_distribution<> map_type_distrib;    


   void generate_cops_and_robbers(){
      if (num_of_cops >= size * size - 1){
         cout << "Error: Too many cops for the board size." << endl;
         return;
      }

      int cops_generated = 0;
      while (cops_generated < num_of_cops){ //gerar policiais até atingir o número
         int new_x = this->get_random_position(); //posições aleatórias
         int new_y = this->get_random_position();

         if (this->position_is_free(new_x,new_y)){ //posição está livre
            this->cells[new_x][new_y] = BoardState::COP; //coloca o policial
            cops_generated += 1;
         }
      }

      int robber_x = this->get_random_position(); //posição do ladrão
      int robber_y = this->get_random_position();

      while (!this->position_is_free(robber_x,robber_y)){ //enquanto a posição gerada não for de um lugar vazio
            robber_x = this->get_random_position();
            robber_y = this->get_random_position();
      }
      this->cells[robber_x][robber_y] = BoardState::ROBBER;
   }

   void generate_walls(){
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

   Board(const int size,const int num_of_cops) //construtor da classe
        : size(size), 
          num_of_cops(num_of_cops),
          cells(size, vector<BoardState>(size, BoardState::EMPTY)), 
          generator(static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count())),                             
          map_elements_distrib(0, size - 1),
          map_type_distrib(1, 2)                  
    {

      if (size < 10){
         cout << "Tamanho mínimo do mapa é 10";
      }
      cout << "Constructor called with size " << size << endl;
      this->generate_walls();
      this->generate_cops_and_robbers();
    }

   void draw_board(){
            string board_string;
            for (int i = 0; i < size; i++){
                  for (int j = 0; j < size; j++){
                        board_string += " ";
                        board_string += (cells[i][j]);
                  }
                  board_string += "\n";
            }
            cout << board_string;
      }

   bool position_is_free(const int x, const int y){
            if (cells[x][y] == BoardState::EMPTY)
               return true;
            return false;
      }
   
   int get_random_position() {
        return map_elements_distrib(generator);
   }

};

int main(){
   Board my_board = Board(15,2);
   my_board.draw_board();
}