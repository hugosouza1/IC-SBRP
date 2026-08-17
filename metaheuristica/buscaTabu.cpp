#include "metaheuristica.hpp"


int distancia(vector<int>& caminho, vector<vector<int>>& grafo){
    int soma = 0;
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

vector<vector<int>> Metaheuristica::caminhosIniciais(vector<vector<int>> conjuntoParadas, vector<bool>& sucesso){
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

Movimento Metaheuristica::melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu, 
    int>& tabu, int melhorDistanciaGlobal, int distanciaAtual, int iteracao ){

    Movimento melhor;
    melhor.tipo = INSERIR;
    melhor.delta = numeric_limits<int>::max();

    for(int pos = 0; pos + 1 < (int)rota.size(); ++pos){

        int A = rota[pos];
        int B = rota[pos + 1];

        for(int C = 1; C < problema.quantidadeParadas; ++C){

            // if(estaNaRota[C]) continue;

            if(problema.grafoParadas[A][C] == 0 || problema.grafoParadas[C][B] == 0)
                continue;

            int delta =
                problema.grafoParadas[A][C] +
                problema.grafoParadas[C][B] -
                problema.grafoParadas[A][B];

            bool tabuAtivo = movimentoTabu(tabu, INSERIR, A, C, B, iteracao);

            bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

            if((!tabuAtivo || aspiracao) &&
               delta < melhor.delta){

                melhor.delta = delta;
                melhor.posicao = pos + 1;

                melhor.anterior = A;
                melhor.parada = C;
                melhor.proximo = B;
            }
        }
    }

    return melhor;
}

Movimento Metaheuristica::melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, 
    unordered_map<ChaveTabu, int>& tabu, int melhorDistanciaGlobal, int distanciaAtual, int iteracao){
    
    Movimento melhor;
    melhor.tipo = REMOVER;
    melhor.delta = numeric_limits<int>::max();

    for(int i = 1; i + 1 < (int)rota.size(); i++){
        int A = rota[i-1], B = rota[i], C = rota[i+1];

        if(paradaObrigatoria[B]) continue;
        if(problema.grafoParadas[A][C] == 0) continue; // precisa existir A->C direto
        
        // bota AC e tira AB e BC
        int delta = problema.grafoParadas[A][C] - problema.grafoParadas[A][B] - problema.grafoParadas[B][C];

        bool tabuAtivo = movimentoTabu(tabu, REMOVER, A, B, C, iteracao);

        bool aspiracao =
            (distanciaAtual + delta) < melhorDistanciaGlobal;

        if((!tabuAtivo || aspiracao) &&
           delta < melhor.delta){

            melhor.delta = delta;
            melhor.posicao = i;

            melhor.anterior = A;
            melhor.parada = B;
            melhor.proximo = C;
        }
    }
    return melhor;
}

Movimento Metaheuristica::melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota,
    vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, int melhorDistanciaGlobal, int iteracao){

    int distanciaAtual = distancia(rota, problema.grafoParadas);

    Movimento ins = melhorInsercao(rota, estaNaRota, tabu, melhorDistanciaGlobal, distanciaAtual,iteracao);

    Movimento rem = melhorRemocao(rota, paradaObrigatoria, tabu, melhorDistanciaGlobal, distanciaAtual, iteracao);

    Movimento melhor = ins;
    if(rem.delta < melhor.delta) melhor = rem;

    return melhor;
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
    }
}


ChaveTabu Metaheuristica::chaveTabu(TipoMovimento tipo, int anterior, int parada, int proximo){
    ChaveTabu chave = 0;

    chave |= ((ChaveTabu)tipo     << (64 - 1)); // 1 bit
    chave |= ((ChaveTabu)anterior << (64 - 1 - 21)); // 21 bit
    chave |= ((ChaveTabu)parada   << (64 - 1 - 21 * 2)); // 21 bit
    chave |= (ChaveTabu)proximo; // 21 bit

    return chave;
}

bool Metaheuristica::movimentoTabu(unordered_map<ChaveTabu, 
    int>& tabu, TipoMovimento tipo, int anterior, int parada, int proximo, int iteracao){

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
    else
        tipoReverso = INSERIR;

    ChaveTabu chave = chaveTabu(tipoReverso, mov.anterior, mov.parada, mov.proximo);

    tabu[chave] = iteracao + tenure;
}
        
void Metaheuristica::pertubacaoRota(vector<vector<int>> &paradasRotas, vector<double> intensidade){
    for(size_t r = 0; r < paradasRotas.size(); r++){
        if(paradasRotas[r].size() < 2) continue;

        uniform_int_distribution<int> dist(0, paradasRotas[r].size() - 1);
        int a = dist(gen);
        int b = dist(gen);
        swap(paradasRotas[r][a], paradasRotas[r][b]);
    }
}

double Metaheuristica::buscaTabu(Individuo& configParada){

    configParada.rotaViavel.assign(quantidadeMaxRota, true);
    int alunosAfetados = 0;

    vector<unordered_set<int>> paradasDaRotaSetAux(quantidadeMaxRota);
    vector<int> alunosPorRota(quantidadeMaxRota, 0);

    for(int aluno = 0; aluno < problema.quantidadeAlunos; aluno++){
        int rota   = configParada.atrAlunoRota[aluno];
        int parada = configParada.atrAlunoParada[aluno];
        paradasDaRotaSetAux[rota].insert(parada);
        alunosPorRota[rota]++;
    }

    vector<vector<int>> paradasDaRota(quantidadeMaxRota);
    for (int r = 0; r < quantidadeMaxRota; ++r) {
        paradasDaRota[r].assign(paradasDaRotaSetAux[r].begin(), paradasDaRotaSetAux[r].end());
    }

    vector<bool> sucessoConstrucao;
    vector<vector<int>> rotasIniciais = caminhosIniciais(paradasDaRota, sucessoConstrucao);

    vector<vector<int>> rotasFinais(rotasIniciais.size());
    double distanciaTotal = 0.0;

    for(int r = 0; r < rotasIniciais.size(); ++r){

        if(!sucessoConstrucao[r]){
            rotasFinais[r] = {};
            configParada.rotaViavel[r] = false;
            alunosAfetados += alunosPorRota[r];
            continue;
        }
        vector<int> rotaAtual = rotasIniciais[r];

        if(rotaAtual.empty()){ rotasFinais[r] = rotaAtual; continue; }

        vector<bool> estaNaRota(problema.quantidadeParadas, false);
        vector<bool> paradaObrigatoria(problema.quantidadeParadas, false);

        for(int p : rotaAtual)        estaNaRota[p] = true;
        for(int p : paradasDaRota[r]) paradaObrigatoria[p] = true;

        unordered_map<ChaveTabu, int> tabu;
        
        int tenure = max(5, (int)(rotaAtual.size() * 2));  // depois ver com calma um melhor
        // int tenure =  10;

        vector<int> melhorRota = rotaAtual;
        int melhorDistancia = distancia(rotaAtual, problema.grafoParadas);

        int it = 0, itSemMelhora = 0;
        const int maxIter = 5000, maxSemMelhora = 100;

        while(it < maxIter && itSemMelhora < maxSemMelhora){

            Movimento mov = melhorVizinho(rotaAtual, estaNaRota, paradaObrigatoria, tabu, melhorDistancia, it);

            if(mov.delta == numeric_limits<int>::max()){
                break;
            }

            aplicaMovimento(rotaAtual, estaNaRota, mov);

            atualizaTabu(tabu, mov, tenure, it);

            int distAtual = distancia(rotaAtual, problema.grafoParadas);

            if(distAtual < melhorDistancia){
                melhorDistancia = distAtual;
                melhorRota = rotaAtual;
                itSemMelhora = 0;
            } else {
                itSemMelhora++;
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