#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <mutex>

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
      random_device rd; 
      uniform_int_distribution<> map_elements_distrib;    
      uniform_int_distribution<> map_type_distrib;    


   void generate_cops_and_robbers(){
      if (num_of_cops >= size * size - 1){
         cout << "Error: Too many cops for the board size." << endl;
         return;
      }
      int cops_generated = 0;
      int max_attempts = size * size * 2;
      while (cops_generated < num_of_cops && max_attempts > 0){ //gerar policiais até atingir o número
         int new_x = this->get_random_position(); //posições aleatórias
         int new_y = this->get_random_position();

         if (this->position_is_free(new_x,new_y)){ //posição está livre
            this->cells[new_x][new_y] = BoardState::COP; //coloca o policial
            cops_generated += 1;
         }
         max_attempts--;
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
        switch (map_choice) {
            case 1:
                generate_border_walls();
                break;
            case 2:
                generate_cross_with_openings();
                break;
            case 3:
                generate_x_pattern();
                break;
            default:
                generate_border_walls(); // Default to pattern 1 if there's an error
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
        int mid_start = size / 2 - 1;  // Start of the 2x2 center
        int mid_end = size / 2;        // End of the 2x2 center

        for (int i = 2; i < size - 2; i++) { // Start from index 2 and stop before size-2 for 2-cell edge spacing
            if (i < mid_start || i > mid_end) { // Skip the 2x2 opening in the center
                cells[i][mid_start] = WALL;   // Vertical line of the cross
                cells[i][mid_end] = WALL;
                cells[mid_start][i] = WALL;   // Horizontal line of the cross
                cells[mid_end][i] = WALL;
            }
        }
    }

    // Pattern 3: X shape with 2-cell spacing from the edges and a 2x2 opening in the center
    void generate_x_pattern() {
        int mid_start = size / 2 - 1;  // Start of the 2x2 center
        int mid_end = size / 2;        // End of the 2x2 center

        for (int i = 2; i < size - 2; i++) { // Start from index 2 and stop before size-2 for 2-cell edge spacing
            if (i < mid_start || i > mid_end) { // Skip the 2x2 opening in the center
                cells[i][i] = WALL;                  // Top-left to bottom-right diagonal
                cells[i][size - 1 - i] = WALL;       // Top-right to bottom-left diagonal
            }
        }
    }

   public:

   Board(int size) //construtor da classe
        : size(size), 
          cells(size, vector<BoardState>(size, BoardState::EMPTY)), 
          generator(rd()),                             
          map_elements_distrib(0, size - 1),
          map_type_distrib(1, 3)                  
    {
    cout << "Constructor called with size " << size << endl;

    }

   void draw_board(){
            string board_string;
            for (int i = 0; i < size; i++){
                  for (int j = 0; j < size; j++){
                        board_string += cells[i][j];
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
   Board my_board = Board(4);
}