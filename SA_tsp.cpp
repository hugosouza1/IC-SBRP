#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <numbers>
#include <chrono>

using namespace std;

int pesoTotal(vector<int>& caminho, vector<vector<int>>& peso){
    int soma = 0;
    for (int i = 0; i + 1 < (int)caminho.size(); i++) {
        soma += peso[caminho[i]][caminho[i+1]];
    }

    soma += peso[caminho.back()][caminho.front()];
    return soma;
}

pair<int,vector<vector<int>>> leitura(){
    vector<pair<int, int>> coorde;
    int x, y, z;
    
    while (cin >> x >> y >> z) {
        coorde.push_back(make_pair(y, z));
    }
    
    vector<vector<int>> grafo(coorde.size(), vector<int>(coorde.size(), -1));
    
    for(int i = 0; i < grafo.size(); i++){
        for(int j = 0; j < grafo.size(); j++){
            if(i == j){
                grafo[i][j] = 0;
                continue;
            }
            double xd = coorde[i].first - coorde[j].first; 
            double yd = coorde[i].second - coorde[j].second; 
            grafo[i][j] = (int)( (sqrt( xd*xd + yd*yd)) + 0.5 );
        }
    }

    return make_pair(coorde.size(), grafo);
}

vector<int> inicialAleatorio(int quantidade){
    vector<int> caminho(quantidade, 0);

    for (int u = 0; u < quantidade; u++) {
        caminho[u] = u;
    }

    sort(caminho.begin(), caminho.end());
    
    do {
        int ran = rand() % 1000;
        if(ran < 350) break;  // 35%
    } while (next_permutation(caminho.begin(), caminho.end()));

    return caminho;
}

// definção da temperuta inicial como um valor que faz com que tenha 80% de 
// chance de aceitar um ruim no inicio.
float tempInicial(vector<int> caminho, vector<vector<int>> grafo){
    int distanciaInicial = pesoTotal(caminho, grafo);

    sort(caminho.begin(), caminho.end());

    vector<int> deltas;
    
    int i = 0, parada = caminho.size() * 2; // quantidade de delta para fazer a media arbitrario
    do {
        int temp = pesoTotal(caminho, grafo);
        if(temp > distanciaInicial){
            deltas.push_back(temp - distanciaInicial);
        }
        i++;
    } while (next_permutation(caminho.begin(), caminho.end()) && i < parada || deltas.empty());
    
    float media = 0;
    for(int j = 0; j < deltas.size(); j++){
        media += deltas[j];
    }
    if(deltas.size() != 0)
    media /= deltas.size();
    
    float chanceRuim = 0.99;

    float tInicial =  media / log(chanceRuim) * - 1; // 80 % de caso ruim a ser aceito no inicio  
    
    return tInicial;
}

int main() {
    using namespace std::chrono;

    auto start = steady_clock::now();         

    auto [quantidade , grafo] = leitura();

    vector<int> caminho = inicialAleatorio(quantidade); 
   
    int SAmax = (quantidade * quantidade); // maximo de iteração
    float temperatura = tempInicial(caminho, grafo);
    const float alfa = 0.80; // taxa de esfriamento
    
    int pesoAtual = pesoTotal(caminho, grafo);
    
    vector<int> melhorCaminho = caminho;
    int melhorDistancia = pesoAtual;

    mt19937 gen(random_device{}());
    uniform_int_distribution<> troca(0, caminho.size() - 1);
    uniform_real_distribution<> aceitarPiora(0.0, 1.0);

    while( temperatura > 0.01){
        for(int i = 0; i < SAmax; i++){
            int primeiro = troca(gen);
            int segundo =  troca(gen);
            while(primeiro == segundo) segundo = troca(gen);

            vector<int> caminhoNovo = caminho;
            swap(caminhoNovo[primeiro], caminhoNovo[segundo]);
            
            int pesoNovo = pesoTotal(caminhoNovo, grafo);
            
            if(pesoAtual > pesoNovo){
                caminho = caminhoNovo;
                pesoAtual = pesoNovo;
                if(melhorDistancia > pesoNovo){
                    melhorCaminho = caminho;
                    melhorDistancia = pesoNovo;
                }
            } else if(aceitarPiora(gen) < exp(double(pesoAtual - pesoNovo) / temperatura)){ 
                caminho = caminhoNovo;
                pesoAtual = pesoNovo;
            }
        }
        temperatura *= alfa; 
    }

    auto end = steady_clock::now();           

    cout << "distancia minima = " << melhorDistancia << "\n";

    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "Tempo: " << ms << " ms\n";
    
    return 0;
}