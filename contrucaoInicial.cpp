#include <iostream>
#include <vector>
#include <numbers>
#include <random>

using namespace std;

int main(){
    vector<vector<int>> matriz(100, vector<int>(100,0));

    // for(int i = 0; i < matriz.size(); i++){
    //     for(int j = 0; j < matriz.size(); j++){
    //         cout << matriz[i][j] << " ";
    //     }
    //     cout << "\n";
    // }

    
    
    mt19937 gen(random_device{}());
    uniform_int_distribution<> linhaEscola((int)(matriz.size() / 3), (int)((matriz.size() * 2 ) / 3));
    uniform_int_distribution<> colunaEscola((int)(matriz.size() / 3), (int)((matriz.size() * 2 ) / 3));

    
}