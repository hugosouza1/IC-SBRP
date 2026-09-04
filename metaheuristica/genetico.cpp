#include "metaheuristica.hpp"

Individuo Metaheuristica::geraSolucaoInicial(){

    Individuo fulano;

    fulano.atrAlunoParada.resize(problema.quantidadeAlunos);
    fulano.atrAlunoRota.resize(problema.quantidadeAlunos);

    vector<int> cargaRota(quantidadeMaxRota, 0);

    for(const auto& aluno : problema.alunosParadas){

        // Escolhe uma parada possível
        uniform_int_distribution<int> distParada(0, aluno.paradasPossiveis.size()-1);

        int parada = aluno.paradasPossiveis[distParada(gen)].first;

        fulano.atrAlunoParada[aluno.id] = parada;

        // Procura rotas que ainda têm capacidade
        vector<int> candidatas;

        for(int r = 0; r < quantidadeMaxRota; r++){

            if(cargaRota[r] < problema.Q)
                candidatas.push_back(r);
        }

        int rota;

        if(candidatas.empty()){
            // nenhuma rota tem capacidade sobrando: cai pra menos carregada,
            // mesmo estourando Q, em vez de acessar candidatas[-1]
            rota = 0;
            for(int r = 1; r < quantidadeMaxRota; r++)
                if(cargaRota[r] < cargaRota[rota]) rota = r;
        } else {
            uniform_int_distribution<int> distRota(0, candidatas.size()-1);
            rota = candidatas[distRota(gen)];
        }

        fulano.atrAlunoRota[aluno.id] = rota;
        cargaRota[rota]++;
    }
    

    fulano.intensidadePermutaRota.assign(quantidadeMaxRota, 0.5);

    return fulano;
}


vector<Individuo> Metaheuristica::popIni(int tamanhoPopulacao){
    vector<Individuo> pop(tamanhoPopulacao);
    for(int i = 0; i < tamanhoPopulacao; ++i){
        pop[i] = geraSolucaoInicial();
    }
    return pop;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

bool Metaheuristica::maisViavel(const Individuo &a, const Individuo &b){
    if(a.alunosInviaveisQuant != b.alunosInviaveisQuant)
        return a.alunosInviaveisQuant < b.alunosInviaveisQuant;  // menos alunos prejudicados vence 
        // cout << "bacate=";
    return a.fitness < b.fitness;
}

int Metaheuristica::selecionaTorneio(vector<Individuo> &populacao, double chanceAceitarPior){
    int n = populacao.size();
    int k = max(1, (int)(n * 0.05));

    uniform_int_distribution<int> torneio(0, n-1);
    uniform_real_distribution<double> sorte(0.0, 1.0);

    int melhorIdx = torneio(gen);
    for(int i = 0; i < k; ++i){
        int idx = torneio(gen);
        bool venceu = maisViavel(populacao[idx], populacao[melhorIdx]);
        bool aceitaMesmoAssim = !venceu && sorte(gen) < chanceAceitarPior;

        if(venceu || aceitaMesmoAssim)
            melhorIdx = idx;
    }
    return melhorIdx;
}

vector<pair<int,int>> Metaheuristica::escolhendoPais(vector<Individuo> &populacao){
    int tamanhoPopulacao = populacao.size();
    
    vector<pair<int,int>> paisEscolhidos;
    paisEscolhidos.reserve(tamanhoPopulacao);

    for(int k = 0; k < tamanhoPopulacao; ++k){
        int pai1 = selecionaTorneio(populacao, 0.3);
        int pai2 = selecionaTorneio(populacao, 0.3);
        int tent = 0;
        while(pai2 == pai1 && tent < 4){
            pai2 = selecionaTorneio(populacao, 0.3);
            ++tent;
        }
        paisEscolhidos.emplace_back(pai1, pai2);  
    }
    return paisEscolhidos;
}

vector<Individuo> Metaheuristica::reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao, int tamanhoPopulacao, double mutacao, double crossoverProb){
    vector<Individuo> filhos;
    filhos.reserve(tamanhoPopulacao);

    int gene = populacao[0].atrAlunoParada.size(); // quantidade de alunos

    uniform_int_distribution<int> corte(0, max(1, gene - 1));

    uniform_real_distribution<double> muta(0.0, 1.0);
    uniform_real_distribution<double> zeroUm(0.0, 1.0);

    for (int i = 0; i < tamanhoPopulacao; ++i){
        int idxPai1 = paisEscolhidos[i].first;
        int idxPai2 = paisEscolhidos[i].second;

        Individuo &pai1 = populacao[idxPai1];
        Individuo &pai2 = populacao[idxPai2];

        Individuo filho; // parada e rota devem ser herdadas do mesmo pai, caso contrário fica quebrado
        filho.atrAlunoParada.resize(gene);
        filho.atrAlunoRota.resize(gene);
        filho.intensidadePermutaRota.resize(quantidadeMaxRota);
        
        // UX - Uniform Crossover
        if (zeroUm(gen) < crossoverProb){

            for(int c = 0; c < gene; c++){
                if(zeroUm(gen) < 0.5){ // 50/50 de herdar de um pai
                    filho.atrAlunoParada[c] = pai1.atrAlunoParada[c];
                    filho.atrAlunoRota[c] = pai1.atrAlunoRota[c];
                } else {
                    filho.atrAlunoParada[c] = pai2.atrAlunoParada[c];
                    filho.atrAlunoRota[c] = pai2.atrAlunoRota[c];
                }
            }
        } else {
            // sem crossover: copia aleatoriamente um dos pais
            if (zeroUm(gen) < 0.5) filho = pai1; // 50 50
            else filho = pai2;
        }

        vector<int> cargaRota(quantidadeMaxRota, 0); 
        for(int g = 0; g < gene; ++g) cargaRota[filho.atrAlunoRota[g]]++;

        //  Assegura um filho factivel. Remanejo de aluno
        for(int g = 0; g < gene; ++g){
            int rotaAtual = filho.atrAlunoRota[g];

            if(cargaRota[rotaAtual] > problema.Q){
                vector<int> candidatas;

                for(int r = 0; r < quantidadeMaxRota; ++r){
                    if(cargaRota[r] < problema.Q)
                        candidatas.push_back(r);
                }

                if(!candidatas.empty()){

                    uniform_int_distribution<int> dist(0, candidatas.size() - 1);
                    int novaRota = candidatas[dist(gen)];

                    cargaRota[rotaAtual]--;
                    cargaRota[novaRota]++;

                    filho.atrAlunoRota[g] = novaRota;
                }
            }
        }

        //  Mutação
        for(int g = 0; g < gene; ++g){
            // Mutação da parada
            if(muta(gen) < mutacao){
                const estudante& aluno = problema.alunosParadas[g];

                if(!aluno.paradasPossiveis.empty()){
                    uniform_int_distribution<int> distParada(0, aluno.paradasPossiveis.size() - 1);
                    filho.atrAlunoParada[g] = aluno.paradasPossiveis[distParada(gen)].first;
                }
            }

            // Mutação da rota
            if(muta(gen) < mutacao){
                int rotaAtual = filho.atrAlunoRota[g];
                vector<int> candidatas;

                for(int r = 0; r < quantidadeMaxRota; ++r){
                    if(r == rotaAtual)
                        continue;

                    if(cargaRota[r] < problema.Q)
                        candidatas.push_back(r);
                }

                if(!candidatas.empty()){
                    uniform_int_distribution<int> distRota(0, candidatas.size() - 1);
                    int novaRota = candidatas[distRota(gen)];

                    cargaRota[rotaAtual]--;
                    cargaRota[novaRota]++;

                    filho.atrAlunoRota[g] = novaRota;
                }
            }
        }
        

        for(int r = 0; r < quantidadeMaxRota; r++){
            double alpha = uniform_real_distribution<double>(0.0, 1.0)(gen);
            filho.intensidadePermutaRota[r] =
                alpha * pai1.intensidadePermutaRota[r] +
                (1.0 - alpha) * pai2.intensidadePermutaRota[r]; // 1.0 fica mais equilibrado
        }


        filhos.push_back(move(filho)); 
    }
    return filhos;
}

vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais, int tamanhoPopulacao, int elitismo){

    vector<Individuo> combinado = filhos;
    combinado.insert(combinado.end(), pais.begin(), pais.end());

    sort(combinado.begin(), combinado.end(), [this](const Individuo &a, const Individuo &b){
        return maisViavel(a, b);
    });

    vector<Individuo> novaPop(combinado.begin(), combinado.begin() + elitismo);
    vector<Individuo> resto(combinado.begin() + elitismo, combinado.end());

    while(novaPop.size() < (size_t)tamanhoPopulacao){
        int idxRo = selecionaTorneio(resto, 0.1); // não ter inviável sobrevivendo? ver com calma
        novaPop.push_back(resto[idxRo]);
    }
    return novaPop;
}


void compactarRotas(Individuo& individuo) {

    vector<vector<int>> novasRotas;
    vector<int> mapa(individuo.rotasFeitas.size(), -1);

    for(int i = 0; i < individuo.rotasFeitas.size(); ++i) {

        if(individuo.rotasFeitas[i].empty())
            continue;

        mapa[i] = novasRotas.size();
        novasRotas.push_back(move(individuo.rotasFeitas[i]));
    }

    for(int e = 0; e < individuo.atrAlunoRota.size(); ++e) {

        int rota = individuo.atrAlunoRota[e];

        if(rota >= 0 && rota < mapa.size()) {
            individuo.atrAlunoRota[e] = mapa[rota];
        }
    }

    individuo.rotasFeitas = move(novasRotas);
}


Individuo Metaheuristica::AG(){

    int maxGeracao = 200;
    int tamanhoPopulacao = 200;
    double crossoverProb = 0.8; 
    double mutacaoProb = 0.08; // baixa mutacao é melhor
    int elitismo = max(3, int(tamanhoPopulacao * 0.05)); // pelo menos 1 // elitismo mais baixo émellhr
    
    vector<Individuo> populacao = popIni(tamanhoPopulacao);

    // inicializa melhor
    vector<double> fitnessInit(tamanhoPopulacao);

    for(int i = 0; i < tamanhoPopulacao; i++){
        fitnessInit[i] = buscaTabu(populacao[i]);
        // cout << "\t\tbacate\n"; fflush(stdin);
    } 

    int idxInit = 0;
    for(int i = 1; i < tamanhoPopulacao; ++i)
        if(maisViavel(populacao[i], populacao[idxInit])) idxInit = i;
    
    Individuo melhorIndividuo = populacao[idxInit];
    double melhorFitness = fitnessInit[idxInit];
    double estagnado = 0;
    
    // max iter e estagnação
    for(int i = 0; i < maxGeracao && estagnado <= 50 + i; ++i){

        vector<pair<int,int>> paisEscolhidos = escolhendoPais(populacao);
        
        vector<Individuo> filhos = reproducao(paisEscolhidos, populacao, tamanhoPopulacao, mutacaoProb, crossoverProb);

        for (auto& filho : filhos) buscaTabu(filho); // fit
        
        // ja chega ordenado
        vector<Individuo> novaPopulacao = novaPopTorneioElitista(filhos, populacao, tamanhoPopulacao, elitismo);

        if (maisViavel(novaPopulacao[0], melhorIndividuo)){
            melhorIndividuo = novaPopulacao[0];
            melhorFitness = novaPopulacao[0].fitness;
            estagnado = i;
        }

        populacao.swap(novaPopulacao);

        if ((i % 10) == 0){
            cout << "Geracao:" << i << " // melhor fitness atual = " << melhorFitness << " / " << melhorFitness << "\n";
        }
    }
    
    
    compactarRotas(melhorIndividuo);

    return melhorIndividuo;
}
