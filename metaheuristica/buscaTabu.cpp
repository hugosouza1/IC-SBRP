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

vector<Movimento> Metaheuristica::candidatosInsercao(vector<int>& rota, vector<bool>& estaNaRota,
    unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao){

    vector<Movimento> candidatos;

    for(int pos = 0; pos + 1 < (int)rota.size(); ++pos){

        int A = rota[pos];
        int B = rota[pos + 1];

        for(int C = 1; C < problema.quantidadeParadas; ++C){

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
                candidatos.push_back(m);
            }
        }
    }

    return candidatos;
}

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
            m.parada   = max(noA, noB);
            m.proximo  = 0;
            candidatos.push_back(m);
        }
    }

    return candidatos;
}

Movimento Metaheuristica::melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota,
    vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, int iteracao){

    double distanciaAtual = distancia(rota, problema.grafoParadas);

    vector<Movimento> candidatos = candidatosInsercao(rota, estaNaRota, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    // cout << "\n" << candidatos.size() << "";
    vector<Movimento> rem =        candidatosRemocao(rota, paradaObrigatoria, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    
    vector<Movimento> tr =         candidatosTroca(rota, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);
    
    candidatos.insert(candidatos.end(), rem.begin(), rem.end());
    // cout << " " << candidatos.size() << " ";
    candidatos.insert(candidatos.end(), tr.begin(), tr.end());
    // cout << "" << candidatos.size() << "\n";

    Movimento nenhum;
    nenhum.delta = numeric_limits<double>::max();

    if(candidatos.empty()) return nenhum;
    


    // +++

    sort(candidatos.begin(), candidatos.end(), [](const Movimento& a, const Movimento& b){
        return a.delta < b.delta;
    });
    
    const int tamanhoRCL = 150;
    int limite = min((int)candidatos.size(), tamanhoRCL);

    // 
    // double deltaMin = candidatos[0].delta;

    // vector<double> pesos(limite);
    // const double temperatura = 10.0; // maior = mais uniforme; menor = mais guloso

    // for(int i = 0; i < limite; ++i){
    //     double diferenca = candidatos[i].delta - deltaMin; // sempre >= 0
    //     pesos[i] = exp(-diferenca / temperatura);
    // }

    // discrete_distribution<int> distPeso(pesos.begin(), pesos.end());
    // return candidatos[distPeso(gen)];


    vector<double> pesos(limite);
    const double decaimento = 0.45; // menor = mais concentrado nos melhores; maior = mais uniforme

    for(int i = 0; i < limite; ++i){
        pesos[i] = exp(-decaimento * i); // rank 0 tem peso 1, rank 1 tem peso menor, etc.
    }

    discrete_distribution<int> distPeso(pesos.begin(), pesos.end());
    return candidatos[distPeso(gen)];
}

void Metaheuristica::aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov){
    switch(mov.tipo){
        case INSERIR:
            rota.insert(rota.begin() + mov.posicao, mov.parada);
            estaNaRota[mov.parada] = true;
            break;

        case REMOVER:
            rota.erase(rota.begin() + mov.posicao);
            estaNaRota[mov.parada] = false;
            break;

        case  TROCAR:
            swap(rota[mov.trocaA], rota[mov.trocaB]);
            break;
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

    if(mov.tipo == INSERIR)
        tipoReverso = REMOVER;
    else if(mov.tipo == REMOVER)
        tipoReverso = INSERIR;
    else 
        tipoReverso = TROCAR;

    ChaveTabu chave;

    chave = chaveTabu(tipoReverso, mov.anterior, mov.parada, mov.proximo);

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
        
        int tenure = max(5, (int)(rotaAtual.size() / 10));  // depois ver com calma um melhor

        // int tenure =  3;
        // cout << "<tt: " << tenure << ", " <<rotaAtual.size() <<  ">";

        vector<int> melhorRota = rotaAtual;
        double melhorDistancia = distancia(rotaAtual, problema.grafoParadas);

        int it = 0;
        const int maxIter = rotaAtual.size() * 3;
        // const int maxIter = 100;

        while(it < maxIter){

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