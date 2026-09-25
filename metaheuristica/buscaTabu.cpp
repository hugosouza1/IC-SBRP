#include "metaheuristica.hpp"


double Metaheuristica::distancia(vector<int>& caminho, vector<vector<double>>& grafo){
    double soma = 0;
    for (int i = 0; i + 1 < (int)caminho.size(); i++) {
        soma += grafo[caminho[i]][caminho[i+1]];
    }
    // soma += grafo[caminho.back()][caminho.front()];
    
    return soma;
}


// ---------- geração inicial da rota ----------

vector<int> Metaheuristica::bfs(int a, int b){
    vector<int> pai(problema.quantidadeParadas, -1);
    vector<bool> visitado(problema.quantidadeParadas, false);
    queue<int> fila;

    visitado[a] = true;
    if(b) visitado[0] = true;
    fila.push(a);

    while(!fila.empty()){
        int u = fila.front(); fila.pop();
        if(u == b) break;

        for(int v = 0; v < (int)problema.grafoParadas[u].size(); v++){
            if(!visitado[v] && problema.grafoParadas[u][v] > 0){
                visitado[v] = true;
                pai[v] = u;
                fila.push(v);
            }
        }
    }

    if(!visitado[b]) return {};

    vector<int> caminho;
    for(int v = b; v != -1; v = pai[v]) caminho.push_back(v);
    reverse(caminho.begin(), caminho.end());
    return caminho;
}


vector<int> Metaheuristica::contrucaoRota(vector<int> obrigatorias, bool& sucesso){
    sucesso = true;
    if(obrigatorias.empty()) return {};

    vector<int> rota;
    int atual = 0;

    for(int parada : obrigatorias){
        if(find(rota.begin(), rota.end(), parada) != rota.end()) continue;

        vector<int> trecho = bfs(atual, parada);
        if(trecho.empty()){ sucesso = false; return {}; }  

        if(atual != 0)
            trecho.erase(trecho.begin()); 

        rota.insert(rota.end(), trecho.begin(), trecho.end());

        atual = parada;
    }

    vector<int> volta = bfs(atual, 0);
    if(volta.empty()){ sucesso = false; return {}; }

    volta.erase(volta.begin());
    rota.insert(rota.end(), volta.begin(), volta.end());

    return rota;
}

// ++++++++++----------+++++++++


// vector<int> Metaheuristica::dijkstra(int a, int b){
//     int n = problema.quantidadeParadas;
//     vector<double> dist(n, numeric_limits<double>::max());
//     vector<int> pai(n, -1);
//     vector<bool> visitado(n, false);

//     priority_queue<pair<double,int>, vector<pair<double,int>>, greater<pair<double,int>>> fila;

//     visitado[a] = true;
//     if(b) visitado[0] = true; 

//     dist[a] = 0.0;
//     fila.push({0.0, a});

//     while(!fila.empty()){
//         auto [d, u] = fila.top(); fila.pop();

//         if(d > dist[u]) continue; 
//         if(u == b) break;

//         for(int v = 0; v < (int)problema.grafoParadas[u].size(); v++){
//             if(visitado[v]) continue;

//             double peso = problema.grafoParadas[u][v];
//             if(peso <= 0) continue;

//             double novaDist = dist[u] + peso;
//             if(novaDist < dist[v]){
//                 dist[v] = novaDist;
//                 pai[v] = u;
//                 fila.push({novaDist, v});
//             }
//         }

//         visitado[u] = true;
//     }

//     if(dist[b] == numeric_limits<double>::max()) return {};

//     vector<int> caminho;
//     for(int v = b; v != -1; v = pai[v]) caminho.push_back(v);
//     reverse(caminho.begin(), caminho.end());
//     return caminho;
// }


// vector<int> Metaheuristica::contrucaoRota(vector<int> obrigatorias, bool& sucesso){
//     sucesso = true;
//     if(obrigatorias.empty()) return {};

//     vector<int> rota;
//     int atual = 0;

//     for(int parada : obrigatorias){
//         if(find(rota.begin(), rota.end(), parada) != rota.end()) continue;

//         vector<int> trecho = dijkstra(atual, parada); 

//         if(trecho.empty()){ sucesso = false; return {}; }  

//         if(atual != 0)
//             trecho.erase(trecho.begin()); 

//         rota.insert(rota.end(), trecho.begin(), trecho.end());

//         atual = parada;
//     }

//     vector<int> volta = dijkstra(atual, 0); 

//     if(volta.empty()){ sucesso = false; return {}; }

//     volta.erase(volta.begin());
//     rota.insert(rota.end(), volta.begin(), volta.end());

//     return rota;
// }


// +++++++++++++++++++++++++++++++++++++++++


vector<vector<int>> Metaheuristica::caminhosIniciais(Individuo &configParada, vector<bool>& sucesso, vector<vector<int>> *paradaDasRotas){

    configParada.rotaViavel.assign(quantidadeMaxRota, true);
    
    vector<unordered_set<int>> paradasDaRotaSetAux(quantidadeMaxRota);

    configParada.alunoPorRota.assign(quantidadeMaxRota, 0);
    
    for(int aluno = 0; aluno < problema.quantidadeAlunos; aluno++){
        int rota   = configParada.atrAlunoRota[aluno];
        int parada = configParada.atrAlunoParada[aluno];
        paradasDaRotaSetAux[rota].insert(parada);
        configParada.alunoPorRota[rota]++;
    }

    vector<vector<int>> conjuntoParadas(quantidadeMaxRota);
    for (int r = 0; r < quantidadeMaxRota; ++r) {
        conjuntoParadas[r].assign(paradasDaRotaSetAux[r].begin(), paradasDaRotaSetAux[r].end());
    }

    if(paradaDasRotas) *paradaDasRotas = conjuntoParadas;
    
    pertubacaoRota(conjuntoParadas, configParada.intensidadePermutaRota);

    vector<vector<int>> rotasIniciais(conjuntoParadas.size());
    sucesso.assign(conjuntoParadas.size(), true);

    for(int i = 0; i < (int)conjuntoParadas.size(); ++i){
        bool ok;
        rotasIniciais[i] = contrucaoRota(conjuntoParadas[i], ok);
        sucesso[i] = ok;
    }
    return rotasIniciais;
}


// ---------- geração de vizinhos por tipo de movimento ----------


vector<Movimento> Metaheuristica::candidatosRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria,
    unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;

    for(int i = 1; i + 1 < (int)rota.size(); i++){
        int A = rota[i-1], B = rota[i], C = rota[i+1];

        if(paradaObrigatoria[B]) continue;
        if(problema.grafoParadas[A][C] == 0) continue;

        double delta = problema.grafoParadas[A][C] - problema.grafoParadas[A][B] - problema.grafoParadas[B][C];

        bool tabuAtivo = movimentoTabu(tabu, REMOVER, A, B, C, iteracao);
        bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

        if(!tabuAtivo || aspiracao){
            Movimento m;
            m.tipo = REMOVER;
            m.delta = delta;
            m.posicao = i;
            m.anterior = A;
            m.parada = B;
            m.proximo = C;
            candidatos.push_back(m);
        }
    }

    return candidatos;
}

vector<Movimento> Metaheuristica::candidatosInsercao(vector<int>& rota, vector<bool>& estaNaRota,
    unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;

    for(int pos = 0; pos + 1 < (int)rota.size(); ++pos){

        int A = rota[pos];
        int B = rota[pos + 1];

        for(int C = 1; C < problema.quantidadeParadas; ++C){

            if(estaNaRota[C]) continue;
            if(problema.grafoParadas[A][C] == 0 || problema.grafoParadas[C][B] == 0)
                continue;

            double delta =
                problema.grafoParadas[A][C] +
                problema.grafoParadas[C][B] -
                problema.grafoParadas[A][B];

            bool tabuAtivo = movimentoTabu(tabu, INSERIR, A, C, B, iteracao);
            bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

            if(!tabuAtivo || aspiracao){
                Movimento m;
                m.tipo = INSERIR;
                m.delta = delta;
                m.posicao = pos + 1;
                m.anterior = A;
                m.parada = C;
                m.proximo = B;
                m.paradaNova = C;
                candidatos.push_back(m);
            }
        }
    }

    return candidatos;
}




vector<Movimento> Metaheuristica::candidatosTroca(vector<int>& rota, unordered_map<ChaveTabu, int>& tabu,
    double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;
    if(rota.size() < 4) return candidatos;

    int amostras = rota.size();
    uniform_int_distribution<int> distIdx(1, rota.size() - 2);

    for(int t = 0; t < amostras; ++t){

        int a = distIdx(gen);
        int b = distIdx(gen);
        if(a == b) continue;
        if(a > b) swap(a, b);

        int prevA = rota[a - 1], noA = rota[a], nextA = rota[a + 1];
        int prevB = rota[b - 1], noB = rota[b], nextB = rota[b + 1];

        bool valido;
        if(b == a + 1){
            valido = problema.grafoParadas[prevA][noB] > 0 &&
                     problema.grafoParadas[noA][nextB] > 0;
        } else {
            valido = problema.grafoParadas[prevA][noB] > 0 &&
                     problema.grafoParadas[noB][nextA] > 0 &&
                     problema.grafoParadas[prevB][noA] > 0 &&
                     problema.grafoParadas[noA][nextB] > 0;
        }
        if(!valido) continue;

        double delta =
              (problema.grafoParadas[prevA][noB] + problema.grafoParadas[noB][nextA]
             + problema.grafoParadas[prevB][noA] + problema.grafoParadas[noA][nextB])
            - (problema.grafoParadas[prevA][noA] + problema.grafoParadas[noA][nextA]
             + problema.grafoParadas[prevB][noB] + problema.grafoParadas[noB][nextB]);

        bool tabuAtivo = movimentoTabu(tabu, TROCAR, min(noA, noB), max(noA, noB), 0, iteracao);
        bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

        if(!tabuAtivo || aspiracao){
            Movimento m;
            m.tipo = TROCAR;
            m.delta = delta;
            m.trocaA = a;
            m.trocaB = b;
            m.anterior = min(noA, noB);
            m.parada = max(noA, noB);
            m.proximo = 0;
            candidatos.push_back(m);
        }
    }

    return candidatos;
}


vector<Movimento> Metaheuristica::candidatosSubstituir(vector<int>& rota, vector<bool>& estaNaRota,
    vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu,
    double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;
    const int tentativasPorPosicao = 8;

    uniform_int_distribution<int> distC(1, problema.quantidadeParadas - 1);

    for(int i = 1; i + 1 < (int)rota.size(); ++i){

        int A = rota[i - 1], B = rota[i], C = rota[i + 1];
        if(paradaObrigatoria[B]) continue;

        for(int tent = 0; tent < tentativasPorPosicao; ++tent){
            int D = distC(gen);
            if(D == B || estaNaRota[D]) continue;

            if(problema.grafoParadas[A][D] == 0 || problema.grafoParadas[D][C] == 0)
                continue;

            double delta =
                  problema.grafoParadas[A][D] + problema.grafoParadas[D][C]
                - problema.grafoParadas[A][B] - problema.grafoParadas[B][C];

            bool tabuAtivo = movimentoTabu(tabu, SUBSTITUIR, min(B, D), max(B, D), 0, iteracao);
            bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

            if(!tabuAtivo || aspiracao){
                Movimento m;
                m.tipo = SUBSTITUIR;
                m.delta = delta;
                m.posicao = i;
                m.paradaAntiga = B;
                m.paradaNova = D;
                m.anterior = min(B, D);
                m.parada = max(B, D);
                m.proximo = 0;
                candidatos.push_back(m);
            }
        }
    }

    return candidatos;
}



vector<Movimento> Metaheuristica::candidatosMover(vector<int>& rota, vector<bool>& paradaObrigatoria,
    unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;
    if(rota.size() < 5) return candidatos;

    int n = (int)rota.size();
    const int tentativas = n;

    uniform_int_distribution<int> distOrigem(1, n - 2);
    uniform_int_distribution<int> distDestino(0, n - 2); // gap de destino

    for(int t = 0; t < tentativas; ++t){

        int origem = distOrigem(gen);
        int destino = distDestino(gen);

        // destino é um gap (entre rota[destino] e rota[destino+1]);
        // não pode coincidir com a própria posição de origem
        if(destino == origem - 1 || destino == origem) continue;

        int A = rota[origem - 1], B = rota[origem], C = rota[origem + 1];
        if(paradaObrigatoria[B]) continue;
        if(problema.grafoParadas[A][C] == 0) continue; // remoção precisa ser válida

        int X = rota[destino], Y = rota[destino + 1];
        if(problema.grafoParadas[X][B] == 0 || problema.grafoParadas[B][Y] == 0) continue;

        double deltaRemocao = problema.grafoParadas[A][C] - problema.grafoParadas[A][B] - problema.grafoParadas[B][C];
        double deltaInsercao = problema.grafoParadas[X][B] + problema.grafoParadas[B][Y] - problema.grafoParadas[X][Y];
        double delta = deltaRemocao + deltaInsercao;

        bool tabuAtivo = movimentoTabu(tabu, MOVER, B, X, Y, iteracao);
        bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

        if(!tabuAtivo || aspiracao){
            Movimento m;
            m.tipo = MOVER;
            m.delta = delta;
            m.posOrigem = origem;
            m.posDestino = destino;
            m.paradaNova = B; // a parada que está sendo movida
            m.anterior = B;
            m.parada = X;
            m.proximo = Y;
            candidatos.push_back(m);
        }
    }

    return candidatos;
}

Movimento Metaheuristica::melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota,
    vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, int iteracao){

    double distanciaAtual = distancia(rota, problema.grafoParadas);

    vector<Movimento> candidatos = candidatosInsercao(rota, estaNaRota, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);

    vector<Movimento> rem = candidatosRemocao(rota, paradaObrigatoria, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    candidatos.insert(candidatos.end(), rem.begin(), rem.end());

    vector<Movimento> tr = candidatosTroca(rota, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    candidatos.insert(candidatos.end(), tr.begin(), tr.end());

    vector<Movimento> sub = candidatosSubstituir(rota, estaNaRota, paradaObrigatoria, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    candidatos.insert(candidatos.end(), sub.begin(), sub.end());

    vector<Movimento> mov = candidatosMover(rota, paradaObrigatoria, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    candidatos.insert(candidatos.end(), mov.begin(), mov.end());

    Movimento nenhum;
    nenhum.delta = numeric_limits<double>::max();
    if(candidatos.empty()) return nenhum;

    sort(candidatos.begin(), candidatos.end(), [](const Movimento& a, const Movimento& b){
        return a.delta < b.delta;
    });

    int limite = min((int)candidatos.size(), TamanhoRCL);
    vector<double> pesos(limite);
    for(int i = 0; i < limite; ++i) pesos[i] = exp(-TaxaDecaimentoRCL * i);

    discrete_distribution<int> distPeso(pesos.begin(), pesos.end());
    return candidatos[distPeso(gen)];
}


void Metaheuristica::aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov){
    switch(mov.tipo){

        case INSERIR:
            rota.insert(rota.begin() + mov.posicao, mov.paradaNova);
            estaNaRota[mov.paradaNova] = true;
            break;

        case REMOVER:
            estaNaRota[mov.parada] = false;
            rota.erase(rota.begin() + mov.posicao);
            break;

        case TROCAR:
            swap(rota[mov.trocaA], rota[mov.trocaB]);
            break;

        case SUBSTITUIR:
            estaNaRota[mov.paradaAntiga] = false;
            estaNaRota[mov.paradaNova] = true;
            rota[mov.posicao] = mov.paradaNova;
            break;

        case MOVER: {
            rota.erase(rota.begin() + mov.posOrigem);
            int destinoAjustado = mov.posDestino;
            if(mov.posDestino > mov.posOrigem) destinoAjustado -= 1; // shift após erase
            rota.insert(rota.begin() + destinoAjustado + 1, mov.paradaNova);
            break;
        }
    }
}



ChaveTabu Metaheuristica::chaveTabu(TipoMovimento tipo, int anterior, int parada, int proximo){
    ChaveTabu chave = 0;

    chave |= ((ChaveTabu)tipo     << (64 - 4));            // 4 bit
    chave |= ((ChaveTabu)anterior << (64 - 4 - 21));      // 20 bit
    chave |= ((ChaveTabu)parada   << (64 - 4 - 21 * 2)); // 20 bit
    chave |= (ChaveTabu)proximo;                        // 20 bit

    return chave;
}

bool Metaheuristica::movimentoTabu(unordered_map<ChaveTabu, int>& tabu, 
    TipoMovimento tipo, int anterior, int parada, int proximo, int iteracao){

    ChaveTabu chave = chaveTabu(tipo, anterior, parada, proximo);

    auto it = tabu.find(chave);

    if(it == tabu.end())
        return false;

    return it->second > iteracao;
}

void Metaheuristica::atualizaTabu(unordered_map<ChaveTabu, int>& tabu, Movimento mov, int tenure, int iteracao){
    TipoMovimento tipoReverso;

    switch(mov.tipo){
        case INSERIR:     tipoReverso = REMOVER; break;
        case REMOVER:     tipoReverso = INSERIR; break;
        case TROCAR:      tipoReverso = TROCAR; break;
        case SUBSTITUIR:  tipoReverso = SUBSTITUIR; break; // simétrico: trocar de volta é a msm operação
        case MOVER:       tipoReverso = MOVER; break;       // idem
    }

    ChaveTabu chave = chaveTabu(tipoReverso, mov.anterior, mov.parada, mov.proximo);
    tabu[chave] = iteracao + tenure;
}

       
void Metaheuristica::pertubacaoRota(vector<vector<int>> &paradasRotas, vector<double> intensidade){

    for(size_t r = 0; r < paradasRotas.size(); r++){

        int tamanhoRota = paradasRotas[r].size();
        if(tamanhoRota < 2) continue;

        double intens = (r < intensidade.size()) ? intensidade[r] : 0.5;

        int maxSwaps = max(1, tamanhoRota / 2);
        int quantSwaps = 1 + (int)round(intens * (maxSwaps - 1));

        uniform_int_distribution<int> dist(0, tamanhoRota - 1);
        
        // cout << intensidade[r] << "\n";

        for(int s = 0; s < quantSwaps; ++s){
            int a = dist(gen);
            int b = dist(gen);
            swap(paradasRotas[r][a], paradasRotas[r][b]);
        }
    }
}

double Metaheuristica::buscaTabu(Individuo& configParada){

    int alunosAfetados = 0;

    vector<bool> sucessoConstrucao;
    vector<vector<int>> paradasDaRota;
    vector<vector<int>> rotasIniciais = caminhosIniciais(configParada, sucessoConstrucao, &paradasDaRota);

    vector<vector<int>> rotasFinais(rotasIniciais.size());
    double distanciaTotal = 0.0;

    for(int r = 0; r < rotasIniciais.size(); ++r){

        if(!sucessoConstrucao[r]){
            rotasFinais[r] = {};
            configParada.rotaViavel[r] = false;
            alunosAfetados += configParada.alunoPorRota[r];
            continue;
        }
        
        vector<int> rotaAtual = rotasIniciais[r];

        if(rotaAtual.empty()){ rotasFinais[r] = rotaAtual; continue; }

        vector<bool> estaNaRota(problema.quantidadeParadas, false);
        vector<bool> paradaObrigatoria(problema.quantidadeParadas, false);

        for(int p : rotaAtual)        estaNaRota[p] = true;
        for(int p : paradasDaRota[r]) paradaObrigatoria[p] = true;

        unordered_map<ChaveTabu, int> tabu;
        
        int tenure = max(3, (int)(rotaAtual.size() * TenureTaxaTamRota));

        // int tenure =  3;
        // cout << "<tt: " << tenure << ", " <<rotaAtual.size() <<  ">";

        vector<int> melhorRota = rotaAtual;
        double melhorDistancia = distancia(rotaAtual, problema.grafoParadas);

        int it = 0;
        // const int maxIter = rotaAtual.size() * 3;
        const int maxIter = IteracoesTabu;

        while(it < maxIter){
            // cout << "yo "; fflush(stdout);

            Movimento mov = melhorVizinho(rotaAtual, estaNaRota, paradaObrigatoria, tabu, melhorDistancia, it);

            if(mov.delta == numeric_limits<double>::max()){
                // travou: sem vizinho admissível.
                bool kickAplicado = false;

                if(rotaAtual.size() >= 4){ // precisa de folga
                    const int maxTentativas = 5;

                    for(int tent = 0; tent < maxTentativas && !kickAplicado; ++tent){
                        uniform_int_distribution<int> distIdx(1, rotaAtual.size() - 2); // evita escola nas pontas

                        int a = distIdx(gen);
                        int b = distIdx(gen);
                        if(a == b) continue;
                        if(a > b) swap(a, b);

                        int prevA = rotaAtual[a - 1], noA = rotaAtual[a], nextA = rotaAtual[a + 1];
                        int prevB = rotaAtual[b - 1], noB = rotaAtual[b], nextB = rotaAtual[b + 1];

                        // checa se as arestas resultantes da troca existem no grafo
                        bool valido;
                        if(b == a + 1){
                            // posições adjacentes: só as arestas das pontas mudam
                            valido = problema.grafoParadas[prevA][noB] > 0 &&
                                     problema.grafoParadas[noA][nextB] > 0;
                        } else {
                            valido = problema.grafoParadas[prevA][noB] > 0 &&
                                     problema.grafoParadas[noB][nextA] > 0 &&
                                     problema.grafoParadas[prevB][noA] > 0 &&
                                     problema.grafoParadas[noA][nextB] > 0;
                        }

                        if(valido){
                            swap(rotaAtual[a], rotaAtual[b]);
                            estaNaRota[rotaAtual[a]] = true;
                            estaNaRota[rotaAtual[b]] = true;
                            kickAplicado = true;
                        }
                    }
                }

                it++;
                continue;
            }

            aplicaMovimento(rotaAtual, estaNaRota, mov);
            atualizaTabu(tabu, mov, tenure, it);

            double distAtual = distancia(rotaAtual, problema.grafoParadas);
            if(distAtual < melhorDistancia){
                melhorDistancia = distAtual;
                melhorRota = rotaAtual;
            }

            it++;
        }

        rotasFinais[r] = melhorRota;
        distanciaTotal += melhorDistancia;
    }

    configParada.alunosInviaveisQuant = alunosAfetados;
    configParada.rotasFeitas = rotasFinais;
    configParada.fitness = distanciaTotal;

    return distanciaTotal;
}