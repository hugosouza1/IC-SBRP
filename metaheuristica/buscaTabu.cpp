#include "metaheuristica.hpp"


int distancia(vector<int>& caminho, vector<vector<int>>& grafo){
    int soma = 0;
    for (int i = 0; i + 1 < (int)caminho.size(); i++) {
        soma += grafo[caminho[i]][caminho[i+1]];
    }
    
    soma += grafo[caminho.back()][caminho.front()];
    return soma;
}

// ---------- geração inicial da rota ----------

vector<int> Metaheuristica::bfs(int a, int b){
    vector<int> pai(problema.quantidadeParadas, -1);
    vector<bool> visitado(problema.quantidadeParadas, false);

    queue<int> fila;

    visitado[a] = true;
    fila.push(a);

    while(!fila.empty()){

        int u = fila.front();
        fila.pop();

        if(u == b)
            break;

        for(int v : problema.grafoParadas[u]){

            if(!visitado[v]){
                visitado[v] = true;
                pai[v] = u;
                fila.push(v);
            }
        }
    }

    vector<int> caminho;

    for(int v = b; v != -1; v = pai[v])
        caminho.push_back(v);

    reverse(caminho.begin(), caminho.end());
     
    return caminho;
}

vector<int> Metaheuristica::contrucaoRota(set<int> paradasMinimas){
    if(paradasMinimas.empty()) return {};

    vector<int> obrigatorias(paradasMinimas.begin(), paradasMinimas.end());

    vector<int> rota;

    int atual = 0;

    for(int parada : obrigatorias){

        vector<int> trecho = bfs(atual, parada);

        if(!rota.empty())
            trecho.erase(trecho.begin());

        rota.insert(rota.end(), trecho.begin(), trecho.end());

        atual = parada;
    }

    vector<int> volta = bfs(atual, 0);

    if(!rota.empty())
        volta.erase(volta.begin());

    rota.insert(rota.end(), volta.begin(), volta.end());

    return rota;
}



vector<vector<int>> Metaheuristica::caminhosIniciais(vector<set<int>> conjuntoParadas){
    vector<vector<int>> rotasInicias(conjuntoParadas.size());

    for(int i = 0; i < conjuntoParadas.size(); ++i){
        rotasInicias[i] = contrucaoRota(conjuntoParadas[i]);
    }
    
    return rotasInicias;
}


// ---------- geração de vizinhos por tipo de movimento ----------

Movimento Metaheuristica::melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual){

    Movimento melhor;
    melhor.tipo = INSERIR;
    melhor.delta = numeric_limits<int>::max();
    
    // isso ae
    // C -> |
    // ^   \/
    // |
    // A -> B
    for(int pos = 0; pos + 1 < rota.size(); ++pos){
        int A = rota[pos];
        int B = rota[pos+1];

        for(int C = 1; C < problema.quantidadeParadas; C++){
            // if(estaNaRota[C]) continue; // ver com carinho depois
            if(problema.grafoParadas[A][C] == 0) continue;
            if(problema.grafoParadas[C][B] == 0) continue;

            int delta = problema.grafoParadas[A][C] + problema.grafoParadas[C][B] - problema.grafoParadas[A][B];

            bool tabuAtivo = tabuParada[C] > 0;
            bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

            if((!tabuAtivo || aspiracao) && delta < melhor.delta){
                melhor.delta    = delta;
                melhor.posicao  = pos + 1; // onde C entra
                melhor.parada   = C;
            }
        }
    }

    return melhor;
}

Movimento Metaheuristica::melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual){
    Movimento melhor;
    melhor.tipo = REMOVER;
    melhor.delta = numeric_limits<int>::max();

    for(int i = 1; i + 1 < (int)rota.size(); i++){
        int A = rota[i-1], B = rota[i], C = rota[i+1];

        if(paradaObrigatoria[B]) continue;
        if(problema.grafoParadas[A][C] == 0) continue; // precisa existir A->C direto
        
        // bota AC e tira AB e BC
        int delta = problema.grafoParadas[A][C] - problema.grafoParadas[A][B] - problema.grafoParadas[B][C];

        bool tabuAtivo = tabuParada[B] > 0;
        bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

        if((!tabuAtivo || aspiracao) && delta < melhor.delta){
            melhor.delta   = delta;
            melhor.posicao = i;
            melhor.parada  = B;
        }
    }
    return melhor;
}

Movimento Metaheuristica::melhorRelocate(vector<int>& rota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual){
    Movimento melhor;
    melhor.tipo = RELOCATE;
    melhor.delta = numeric_limits<int>::max();

    int n = rota.size();
    
    // testar com carinho depois
    // tenta colocar um i na frente do j
    // i = 1(B); j = 4(E);
    // A - B - C - D  - E - F
    // A - C - D - E - B - F
    for(int i = 1; i + 1 < n; i++){
        int Aant = rota[i-1], B = rota[i], Adep = rota[i+1];

        int deltaRemocao = problema.grafoParadas[Aant][Adep] - problema.grafoParadas[Aant][B] - problema.grafoParadas[B][Adep];

        for(int j = 1; j + 1 < n; j++){
            if(j == i || j == i - 1) continue;

            int C = rota[j], D = rota[j+1];
            
            // ver CB e BD
            if( ! (problema.grafoParadas[C][B] * problema.grafoParadas[B][D])) continue; 
            int deltaInsercao = problema.grafoParadas[C][B] + problema.grafoParadas[B][D] - problema.grafoParadas[C][D];

            int delta = deltaRemocao + deltaInsercao;

            bool tabuAtivo = tabuParada[B] > 0;
            bool aspiracao = (distanciaAtual + delta) < melhorDistanciaGlobal;

            if((!tabuAtivo || aspiracao) && delta < melhor.delta){
                melhor.delta       = delta;
                melhor.posicao     = i;
                melhor.parada      = B;
                melhor.novaPosicao = j;
            }
        }
    }
    return melhor;
}

Movimento Metaheuristica::melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal){

    int distanciaAtual = distancia(rota, problema.grafoParadas);

    Movimento ins = melhorInsercao(rota, estaNaRota, tabuParada, melhorDistanciaGlobal, distanciaAtual);
    Movimento rem = melhorRemocao(rota, paradaObrigatoria, tabuParada, melhorDistanciaGlobal, distanciaAtual);
    Movimento rel = melhorRelocate(rota, tabuParada, melhorDistanciaGlobal, distanciaAtual);

    Movimento melhor = ins;
    if(rem.delta < melhor.delta) melhor = rem;
    if(rel.delta < melhor.delta) melhor = rel;

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
        case RELOCATE: {
            rota.erase(rota.begin() + mov.posicao);
            int destino = mov.novaPosicao;
            if(mov.novaPosicao > mov.posicao) destino--; // corrige índice após o erase
            rota.insert(rota.begin() + destino, mov.parada);
            break;
        }
    }
}

void Metaheuristica::atualizaTabu(vector<int>& tabuParada, int paradaMovida, int tenure){
    for(int& t : tabuParada) if(t > 0) t--;
    tabuParada[paradaMovida] = tenure;
}


int Metaheuristica::buscaTabu(Individuo& configParada){

    vector<set<int>> paradasDaRota(quantidadeMaxRota);

    for(int aluno = 0; aluno < problema.quantidadeAlunos; aluno++){
        int rota   = configParada.atrAlunoRota[aluno];
        int parada = configParada.atrAlunoParada[aluno];
        paradasDaRota[rota].insert(parada);
    }

    vector<vector<int>> rotasIniciais = caminhosIniciais(paradasDaRota);

    vector<vector<int>> rotasFinais(rotasIniciais.size());

    int distanciaTotal = 0;

    for(int r = 0; r < rotasIniciais.size(); ++r){

        vector<int> rotaAtual = rotasIniciais[r];
        if(rotaAtual.empty()){ rotasFinais[r] = rotaAtual; continue; }

        vector<bool> estaNaRota(problema.quantidadeParadas, false);
        vector<bool> paradaObrigatoria(problema.quantidadeParadas, false);

        for(int p : rotaAtual)        estaNaRota[p] = true;
        for(int p : paradasDaRota[r]) paradaObrigatoria[p] = true;

        vector<int> tabuParada(problema.quantidadeParadas, 0);
        int tenure = max(3, (int)(rotaAtual.size() * 0.2));  // depois ver com calma um melhor

        vector<int> melhorRota = rotaAtual;
        int melhorDistancia = distancia(rotaAtual, problema.grafoParadas);

        int it = 0, itSemMelhora = 0;
        const int maxIter = 1000, maxSemMelhora = 100;

        while(it < maxIter && itSemMelhora < maxSemMelhora){

            Movimento mov = melhorVizinho(rotaAtual, estaNaRota, paradaObrigatoria, tabuParada, melhorDistancia);

            if(mov.delta == numeric_limits<int>::max()){
                break; // sem vizinho viável. fudeu
                cout << "moiou\n";
            }

            aplicaMovimento(rotaAtual, estaNaRota, mov);
            atualizaTabu(tabuParada, mov.parada, tenure);

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

    configParada.rotasFeitas = rotasFinais;
    configParada.fitness = distanciaTotal;
    
    return distanciaTotal;
}



