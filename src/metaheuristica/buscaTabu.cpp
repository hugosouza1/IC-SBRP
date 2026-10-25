#include "metaheuristica.hpp"


// ---------- distâncias de caminho mínimo ----------

void Metaheuristica::iniciaDistancias(){
    int n = problema.quantidadeParadas;
    vector<vector<double>> grafo(n, vector<double>(n, 0.0));
    for(int i = 0; i < n; ++i)
        for(int j = 0; j < n; ++j)
            grafo[i][j] = problema.grafoParadas[i][j];

    distancias.inicia(grafo);
}

// depósito -> paradas na ordem -> depósito
double Metaheuristica::distanciaSequencia(const vector<int>& paradas){
    if(paradas.empty()) return 0.0;

    double soma = distancias[0][paradas[0]];
    for(int i = 0; i + 1 < (int)paradas.size(); ++i){
        soma += distancias[paradas[i]][paradas[i+1]];
    }
    soma += distancias[paradas.back()][0];

    return soma;
}


// ---------- construção da ordem de visita ----------

// Held-Karp: ótimo exato, O(2^k * k^2). só pra k <= LimiteExato
double Metaheuristica::rotaExata(const vector<int>& paradas, vector<int>& ordem){
    int k = paradas.size();
    int total = 1 << k;

    vector<double> custo((size_t)total * k, INFINITO);
    vector<int> anterior((size_t)total * k, -1);

    auto idx = [&](int mascara, int j){ return (size_t)mascara * k + j; };

    for(int i = 0; i < k; ++i) custo[idx(1 << i, i)] = distancias[0][paradas[i]];

    for(int mascara = 1; mascara < total; ++mascara){
        for(int i = 0; i < k; ++i){
            if(!(mascara >> i & 1)) continue;

            double base = custo[idx(mascara, i)];
            if(base >= INFINITO / 2) continue; // / 2 pq é soma

            for(int j = 0; j < k; ++j){
                if(mascara >> j & 1) continue;

                int nova = mascara | (1 << j);
                double c = base + distancias[paradas[i]][paradas[j]];
                if(c < custo[idx(nova, j)]){
                    custo[idx(nova, j)] = c;
                    anterior[idx(nova, j)] = i;
                }
            }
        }
    }

    double melhor = INFINITO;
    int ultima = -1;
    for(int i = 0; i < k; ++i){
        double c = custo[idx(total - 1, i)] + distancias[paradas[i]][0];
        if(c < melhor){ melhor = c; ultima = i; }
    }

    // reconstrói de trás pra frente
    ordem.clear();
    int mascara = total - 1;
    for(int j = ultima; j != -1; ){
        ordem.push_back(paradas[j]);
        int ant = anterior[idx(mascara, j)];
        mascara ^= (1 << j);
        j = ant;
    }
    reverse(ordem.begin(), ordem.end());

    return melhor;
}


// partida gulosa: paradas mais longe do depósito primeiro, cada uma na posição mais barata
vector<int> Metaheuristica::insercaoMaisBarata(const vector<int>& paradas){
    vector<int> restantes = paradas;

    sort(restantes.begin(), restantes.end(), [&](int a, int b){
        return distancias[0][a] > distancias[0][b];
    });

    vector<int> sequencia;
    for(int parada : restantes){
        int melhorPos = 0;
        double melhorCusto = INFINITO;

        for(int pos = 0; pos <= (int)sequencia.size(); ++pos){
            sequencia.insert(sequencia.begin() + pos, parada);
            double c = distanciaSequencia(sequencia);
            sequencia.erase(sequencia.begin() + pos);

            if(c < melhorCusto){
                melhorCusto = c;
                melhorPos = pos;
            }
        }
        sequencia.insert(sequencia.begin() + melhorPos, parada);
    }

    return sequencia;
}


// ---------- tabu sobre a permutação das paradas ----------
// vizinhança completa (2-opt + or-opt de bloco 1 a 3), tabu por nó, aspiração pelo melhor da trajetória

double Metaheuristica::tabuSequencia(vector<int>& sequencia, int iteracoes){
    int n = sequencia.size();
    if(n < 2) return distanciaSequencia(sequencia);

    vector<int> tabuAte(problema.quantidadeParadas, 0);
    int tenure = max(3, (int)(n * TenureTaxaTamRota));

    double atual = distanciaSequencia(sequencia);
    double melhor = atual;

    vector<int> melhorSequencia = sequencia;
    int semMelhora = 0;

    for(int it = 1; it <= iteracoes; ++it){

        double melhorCusto = INFINITO;
        vector<int> melhorMov;
        int noA = -1, noB = -1;

        auto avalia = [&](const vector<int>& vizinho, int a, int b){
            double c = distanciaSequencia(vizinho);

            bool tabuAtivo = tabuAte[a] > it || tabuAte[b] > it;
            bool aspiracao = c < melhor - 1e-9;
            if(tabuAtivo && !aspiracao) return;

            if(c < melhorCusto){
                melhorCusto = c;
                melhorMov = vizinho;
                noA = a;
                noB = b;
            }
        };

        // 2-opt
        for(int i = 0; i + 1 < n; ++i){
            for(int j = i + 1; j < n; ++j){
                vector<int> vizinho = sequencia;
                reverse(vizinho.begin() + i, vizinho.begin() + j + 1);
                avalia(vizinho, sequencia[i], sequencia[j]);
            }
        }

        // or-opt: tira o bloco [i, i+L) e reinsere em outra posição
        for(int L = 1; L <= 3 && L < n; ++L){
            for(int i = 0; i + L <= n; ++i){

                vector<int> resto;
                resto.reserve(n - L);

                for(int x = 0; x < n; ++x){
                    if(x < i || x >= i + L) resto.push_back(sequencia[x]);
                }

                for(int pos = 0; pos <= (int)resto.size(); ++pos){
                    if(pos == i) continue; // mesma posição

                    vector<int> vizinho = resto;
                    vizinho.insert(vizinho.begin() + pos, sequencia.begin() + i, sequencia.begin() + i + L);
                    avalia(vizinho, sequencia[i], sequencia[i + L - 1]);
                }
            }
        }

        if(melhorMov.empty()) break;

        sequencia = melhorMov;
        atual = melhorCusto;
        tabuAte[noA] = it + tenure;
        tabuAte[noB] = it + tenure;

        if(atual < melhor - 1e-9){
            melhor = atual;
            melhorSequencia = sequencia;
            semMelhora = 0;
        } else semMelhora++;

        // estagnou: volta pro melhor e sacode com swaps aleatórios
        if(semMelhora > LimiarEstagnacaoTabu){
            sequencia = melhorSequencia;

            uniform_int_distribution<int> distPos(0, n - 1);
            int swaps = max(2, n / 4);
            for(int k = 0; k < swaps; ++k) swap(sequencia[distPos(gen)], sequencia[distPos(gen)]);

            fill(tabuAte.begin(), tabuAte.end(), 0);
            semMelhora = 0;
        }
    }

    sequencia = melhorSequencia;
    return melhor;
}


// melhor ordem pra um conjunto de paradas (com cache por conjunto)
// k <= LimiteExato: exato. senão: NumTrajetorias partidas (1ª gulosa, resto aleatória) + tabuSequencia
double Metaheuristica::melhorOrdemRota(vector<int> paradas, vector<int>& ordem){
    sort(paradas.begin(), paradas.end());
    paradas.erase(unique(paradas.begin(), paradas.end()),    paradas.end());
    paradas.erase(remove(paradas.begin(), paradas.end(), 0), paradas.end());

    if(paradas.empty()){ ordem.clear(); return 0.0; }

    string chave((const char*)paradas.data(), paradas.size() * sizeof(int));

    auto it = cacheRotas.find(chave);

    if(it != cacheRotas.end()){
        ordem = it->second.second;
        double custo = it->second.first;

        if((int)paradas.size() > LimiteExato && (uniform_real_distribution<double>(0.0, 1.0)(gen) < 0.1)){
            vector<int> seq = ordem;
            double c = tabuSequencia(seq, IteracoesTabu / max(1, NumTrajetorias));
            if(c < custo - 1e-9){
                custo = c; ordem = seq;
                it->second = {custo, ordem};
            }
        }
        
        return custo;
    }

    double custo;

    if((int)paradas.size() <= LimiteExato){
        custo = rotaExata(paradas, ordem);
    } else {
        custo = INFINITO;
        int iteracoes = max(10, IteracoesTabu / max(1, NumTrajetorias));

        for(int t = 0; t < max(1, NumTrajetorias); ++t){
            vector<int> sequencia = (t == 0) ? insercaoMaisBarata(paradas) : paradas; // qualidade rapida com diversidade

            if(t > 0) shuffle(sequencia.begin(), sequencia.end(), gen);

            double c = tabuSequencia(sequencia, iteracoes);
            if(c < custo){
                custo = c;
                ordem = sequencia;
            }
        }
    }

    cacheRotas[chave] = {custo, ordem};
    return custo;
}

// expande a ordem das paradas no trajeto real ( 0 -> ... -> 0 ) pelos predecessores do Dijkstra
vector<int> Metaheuristica::trajetoReal(const vector<int>& ordem){
    vector<int> alvo = ordem;
    alvo.push_back(0);

    vector<int> trajeto = {0};
    int atual = 0;

    for(int parada : alvo){
        const vector<int>& pai = distancias.predecessoresDe(atual);

        vector<int> trecho;
        for(int v = parada; v != -1; v = pai[v]) trecho.push_back(v);
        reverse(trecho.begin(), trecho.end());

        trajeto.insert(trajeto.end(), trecho.begin() + 1, trecho.end());
        atual = parada;
    }

    return trajeto;
}


// ---------- avaliação do indivíduo ----------

double Metaheuristica::buscaTabu(Individuo& configParada){
    int quantRotas = quantidadeMaxRota;

    configParada.rotaViavel.assign(quantRotas, true);
    configParada.alunoPorRota.assign(quantRotas, 0);
    configParada.rotasFeitas.assign(quantRotas, {});

    vector<set<int>> paradasDaRota(quantRotas);
    for(int aluno = 0; aluno < problema.quantidadeAlunos; ++aluno){
        int rota   = configParada.atrAlunoRota[aluno];
        int parada = configParada.atrAlunoParada[aluno];

        configParada.alunoPorRota[rota]++;
        if(parada != 0) paradasDaRota[rota].insert(parada);
    }

    double distanciaTotal = 0.0;
    int alunosAfetados = 0;

    for(int r = 0; r < quantRotas; ++r){
        if(paradasDaRota[r].empty()) continue;

        // alguma parada sem caminho de ida e volta pro depósito
        bool alcancavel = true;
        for(int p : paradasDaRota[r]){
            if(distancias[0][p] >= INFINITO || distancias[p][0] >= INFINITO){
                alcancavel = false;
                break;
            }
        }

        if(!alcancavel){
            configParada.rotaViavel[r] = false;
            alunosAfetados += configParada.alunoPorRota[r];
            continue;
        }

        vector<int> ordem;
        distanciaTotal += melhorOrdemRota(vector<int>(paradasDaRota[r].begin(), paradasDaRota[r].end()), ordem);
        configParada.rotasFeitas[r] = trajetoReal(ordem);
    }

    configParada.alunosInviaveisQuant = alunosAfetados;
    configParada.fitnessPuro = distanciaTotal;

    return distanciaTotal;
}