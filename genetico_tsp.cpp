#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <utility>
#include <algorithm>
#include <numeric>
#include <chrono>

using namespace std;
using namespace chrono;

// um global fica mais rapido
random_device rd;
mt19937 gen(rd());


int distancia(const vector<int>& caminho, vector<vector<int>>& grafo){
    int soma = 0;
    for (int i = 0; i + 1 < (int)caminho.size(); i++) {
        soma += grafo[caminho[i]][caminho[i+1]];
    }

    soma += grafo[caminho.back()][caminho.front()];
    return soma;
}

void imprimeCaminho(vector<int> &caminho, vector<vector<int>> &grafo){
    int total = 0;
    for(int i = 0; i + 1 < caminho.size(); ++i){
        total += grafo[caminho[i]][caminho[i+1]];
    }
    total += grafo[caminho.back()][caminho.front()];

    cout << "caminho: ";
    for(int i = 0; i < caminho.size(); ++i){
        cout << caminho[i] << "-";
    }
    cout << caminho.front() << "\ndistancia: " << total << "\n";
}

pair<int,vector<vector<int>>> leitura(){
    vector<pair<int, int>> coorde;
    int x, y, z;
    
    while (cin >> x >> y >> z) {
        coorde.push_back(make_pair(y, z));
    }
    
    vector<vector<int>> grafo(coorde.size(), vector<int>(coorde.size(), -1));
    
    for(int i = 0; i < grafo.size(); ++i){
        for(int j = 0; j < grafo.size(); ++j){
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

vector<int> geraSolucaoInicial(int tam){
    vector<int> indices(tam-1);
    iota(indices.begin(), indices.end(), 1);
    
    vector<int> caminho; caminho.push_back(0);
    while(caminho.size() < tam){
        uniform_int_distribution<int> indx(0, indices.size() - 1);

        int indi = indx(gen);
        int valor = indices[indi];
        caminho.push_back(valor);
        indices.erase(remove(indices.begin(), indices.end(), valor), indices.end());
    }
    return caminho;
}

vector<vector<int>> popIni(vector<vector<int>> grafo, int tamanhoPopulacao){
    vector<vector<int>> pop(tamanhoPopulacao);
    for(int i = 0; i < tamanhoPopulacao; ++i){
        pop[i] = geraSolucaoInicial(grafo.size());
    }
    return pop;
}

vector<double> calculaFitness(vector<vector<int>> &pop, vector<vector<int>> grafo){
    int n = pop.size();
    vector<double> fitness(n, 0);

    for(int i = 0; i < n; ++i){
        fitness[i] = 10.0 / (distancia(pop[i], grafo) + 1.0);
    }

    return fitness;
}

int selecionaRoleta(vector<double> &fitness){
    int n = fitness.size();
    vector<double> f = fitness; 

    double soma = accumulate(f.begin(), f.end(), 0.0);

    // fallback: se todos = zero escolhe aleatoriamente uniforme
    if (soma <= 1e-12){
        uniform_int_distribution<int> uni(0, n-1);
        return uni(gen);
    }

    uniform_real_distribution<double> dist(0.0, soma);
    double r = dist(gen);

    double cumul = 0.0;
    for(int i = 0; i < n; ++i){
        cumul += f[i];
        if (r <= cumul) return i;
    }
    return n-1;
}

vector<pair<int,int>> escolhendoPais(vector<double> &fitness, int tamanhoPopulacao){
    vector<pair<int,int>> paisEscolhidos;
    paisEscolhidos.reserve(tamanhoPopulacao);

    for(int k = 0; k < tamanhoPopulacao; ++k){
        int pai1 = selecionaRoleta(fitness);
        int pai2 = selecionaRoleta(fitness);
        int tent = 0;
        while(pai2 == pai1 && tent < 10){
            pai2 = selecionaRoleta(fitness);
            ++tent;
        }
        paisEscolhidos.emplace_back(pai1, pai2); // move direto. melhor pq pair é mais pesado
    }
    return paisEscolhidos;
}

vector<vector<int>> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<vector<int>> &populacao, int tamanhoPopulacao, double mutacao, double crossoverProb){
    vector<vector<int>> filhos;
    filhos.reserve(tamanhoPopulacao);

    int gene = populacao[0].size();
    uniform_int_distribution<int> corte(0, max(1, gene - 1));
    uniform_real_distribution<double> muta(0.0, 1.0);
    uniform_real_distribution<double> zeroUm(0.0, 1.0);

    for (int i = 0; i < tamanhoPopulacao; ++i){
        int idxPai1 = paisEscolhidos[i].first;
        int idxPai2 = paisEscolhidos[i].second;

        vector<int> &pai1 = populacao[idxPai1];
        vector<int> &pai2 = populacao[idxPai2];

        vector<int> filho(gene, -1);
        
        // OX
        if (zeroUm(gen) < crossoverProb){
            int ponto1 = corte(gen);
            int ponto2 = corte(gen);
            while(ponto1 == ponto2) ponto2 = corte(gen); // pego um trecho do dns
            if(ponto1 > ponto2) swap(ponto1, ponto2);

            ////////////
            // copia parte do pai1 e resto do pai2
            copy(pai1.begin() + ponto1, pai1.begin() + ponto2, filho.begin() + ponto1);

            vector<int> restante;
            restante.reserve(gene - (ponto2 - ponto1));
            
            vector<bool> presente(gene, false);
            for (int k = ponto1; k < ponto2; ++k){
                presente[pai1[k]] = true;
            }

            for (int k = 0; k < gene; ++k){
                int idxPai2 = (ponto2 + k) % gene;       
                int val = pai2[idxPai2];

                if (presente[val] == false) restante.push_back(val);
                presente[val] = true;
            }

            int idxRest = 0;
            for (int pos = ponto2; idxRest < restante.size(); ++pos){
                filho[pos % gene] = restante[idxRest++];
            }

        } else {
            // sem crossover: copia aleatoriamente um dos pais
            if (zeroUm(gen) < 0.5) filho = pai1; // 50 50
            else filho = pai2;
        }

        // mutação 
        for (int g = 0; g < gene; ++g){
            if (muta(gen) < mutacao){
                int ponto1 = corte(gen);
                int ponto2 = corte(gen);
                while(ponto1 == ponto2) ponto2 = corte(gen); 
                swap(filho[ponto1], filho[ponto2]);
            }
        }

        filhos.push_back(move(filho)); // sem copia
    }
    return filhos;
}

vector<vector<int>> novaPopRoleta(vector<vector<int>> &filhos, vector<vector<int>> &pais,
                                  vector<vector<int>> grafo, int tamanhoPopulacao, int elitismo){

    vector<vector<int>> combinado = filhos;
    combinado.insert(combinado.end(), pais.begin(), pais.end());

    vector<double> fitnessCombinado = calculaFitness(combinado, grafo);

    int m = combinado.size();
    vector<int> indices(m);
    iota(indices.begin(), indices.end(), 0);

    // ordena índices por fitness decrescente
    sort(indices.begin(), indices.end(), [&](int a, int b){
        return fitnessCombinado[a] > fitnessCombinado[b];
    });

    vector<vector<int>> novaPop;
    novaPop.reserve(tamanhoPopulacao);

    int manter = min(elitismo, tamanhoPopulacao);
    for (int k = 0; k < manter; ++k){
        novaPop.push_back(combinado[indices[k]]);
    }

    // preenche o restante por roleta 
    while ((int)novaPop.size() < tamanhoPopulacao){
        int idx = selecionaRoleta(fitnessCombinado);
        novaPop.push_back(combinado[idx]);
    }

    if ((int)novaPop.size() > tamanhoPopulacao)
        novaPop.resize(tamanhoPopulacao);

    return novaPop;
}


// depois encher de & pra ver se fica mais rapido 
void AG_TSP(){
    auto [quantidade , grafo] = leitura();

    int maxGeracao = 1000;
    int tamanhoPopulacao = 100;
    double crossoverProb = 0.8; 
    double mutacaoProb = 0.01; // baixa mutacao é melhor
    int elitismo = max(1, int(tamanhoPopulacao * 0.05)); // pelo menos 1 // elitismo mais baixo émellhr
    
    vector<vector<int>> populacao = popIni(grafo, tamanhoPopulacao);

    // inicializa melhor
    vector<double> fitnessInit = calculaFitness(populacao, grafo);
    int idxInit = distance(fitnessInit.begin(), max_element(fitnessInit.begin(), fitnessInit.end()));
    
    vector<int> melhorIndividuo = populacao[idxInit];
    double melhorFitness = fitnessInit[idxInit];
    
    for(int i = 0; i < maxGeracao; ++i){
        vector<double> fitness = calculaFitness(populacao, grafo);
        
        vector<pair<int,int>> paisEscolhidos = escolhendoPais(fitness, tamanhoPopulacao);
        
        vector<vector<int>> filhos = reproducao(paisEscolhidos, populacao, tamanhoPopulacao, mutacaoProb, crossoverProb);
        
        vector<vector<int>> novaPopulacao = novaPopRoleta(filhos, populacao, grafo, tamanhoPopulacao, elitismo);
        
        vector<double> novoFitness = calculaFitness(novaPopulacao, grafo);

        int idx = distance(novoFitness.begin(), max_element(novoFitness.begin(), novoFitness.end()));
        
        if (novoFitness[idx] > melhorFitness){
            melhorIndividuo = novaPopulacao[idx];
            melhorFitness = novoFitness[idx];
        }

        populacao.swap(novaPopulacao);

        // if ((i % 10) == 0){
            // cout << "Geracao:" << i << " // melhor fitness atual = " << melhorFitness << " / " << distancia(melhorIndividuo, grafo) << "\n";
        // }
    }
    
    // imprimeCaminho(melhorIndividuo, grafo);
    cout << "distancia: " << distancia(melhorIndividuo, grafo); cout << "\n";

}



int main(){
    auto start = steady_clock::now();   

    AG_TSP();
    
    auto end = steady_clock::now();           
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "Tempo: " << ms << " ms\n";

    return 0;
}