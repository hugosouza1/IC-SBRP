#include "metaheuristica.hpp"


void Metaheuristica::finalizaSolucao(Individuo& configParada, vector<vector<int>>& rotas, const vector<bool>& sucesso) {
    // Garante que o vetor tenha todas as rotas
    configParada.rotasFeitas.assign(quantidadeMaxRota, {});

    double distanciaTotal = 0.0;
    int alunosAfetados = 0;

    for(int r = 0; r < quantidadeMaxRota; ++r){

        // Se a rota não foi construída ou é inviável
        if(r >= (int)rotas.size() || r >= (int)sucesso.size() || !sucesso[r] || rotas[r].empty()) {

            configParada.rotasFeitas[r] = {};
            configParada.rotaViavel[r] = false;
            alunosAfetados += configParada.alunoPorRota[r];
            continue;
        }

        // Rota válida
        configParada.rotasFeitas[r] = rotas[r];
        configParada.rotaViavel[r] = true;

        distanciaTotal += distancia(rotas[r], problema.grafoParadas);
    }

    configParada.alunosInviaveisQuant = alunosAfetados;
    configParada.fitness = distanciaTotal;
}



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
    
    uniform_real_distribution<double> shake(0.3, 0.7);
    fulano.intensidadePermutaRota.resize(quantidadeMaxRota);

    for(int i = 0; i < quantidadeMaxRota; ++i){
        fulano.intensidadePermutaRota[i] = shake(gen);
    }

    return fulano;
}


vector<Individuo> Metaheuristica::popIni(int tamanhoPopulacao){
    vector<Individuo> pop(tamanhoPopulacao);
    for(int i = 0; i < tamanhoPopulacao; ++i){
        pop[i] = geraSolucaoInicial();
    }
    return pop;
}

double Metaheuristica::aplicaPenalidades(Individuo& individuo){

    double penalidade = 0.0;

    // --- penalidade por quantidade de rotas ativas ---
    const double custoPorRotaAtiva = 100.0; 
    const int limiarPequena = 3;
    const double penalidadeExtraPequena = 40.0;

    int rotasAtivas = 0;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(individuo.rotaViavel[r] && individuo.alunoPorRota[r] > 0){
            rotasAtivas++;
            if(individuo.alunoPorRota[r] <= limiarPequena){
                penalidade += penalidadeExtraPequena;
            }
        }
    }
    penalidade += rotasAtivas * custoPorRotaAtiva;

    // --- penalidade por parada repetida entre rotas ---
    const int limiteParadaPorRota = 1;
    const double custoPorRepeticao = 40.0;

    unordered_map<int, int> contagemParada;
    for(const auto& rota : individuo.rotasFeitas){
        unordered_set<int> paradasNestaRota(rota.begin(), rota.end());
        for(int p : paradasNestaRota){
            if(p == 0) continue; // depósito
            contagemParada[p]++;
        }
    }

    for(auto& [parada, quant] : contagemParada){
        if(quant > limiteParadaPorRota){
            penalidade += (quant - limiteParadaPorRota) * custoPorRepeticao;
        }
    }

    individuo.penalidadeFitness = penalidade;
    individuo.fitness += penalidade;

    return penalidade;
}


// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

bool Metaheuristica::maisViavel(const Individuo &a, const Individuo &b){
    if(a.alunosInviaveisQuant || b.alunosInviaveisQuant){
        cerr << "Aluno inviavel\n";
        exit(1);
    }

    int rotasAtivasA = count(a.rotaViavel.begin(), a.rotaViavel.end(), true);
    int rotasAtivasB = count(b.rotaViavel.begin(), b.rotaViavel.end(), true);

    const double pesoRota = 5.0;

    double scoreA = a.fitness + rotasAtivasA * pesoRota;
    double scoreB = b.fitness + rotasAtivasB * pesoRota;

    return scoreA < scoreB;
}

int Metaheuristica::selecionaTorneio(vector<Individuo> &populacao, double chanceAceitarPior){
    int n = populacao.size();
    int k = max(1, (int)(n * 0.02));

    uniform_int_distribution<int> torneio(0, n-1);
    uniform_real_distribution<double> sorte(0.0, 1.0);

    int melhorIdx = torneio(gen);
    for(int i = 0; i < k; ++i){
        int idx = torneio(gen);
        bool venceu = maisViavel(populacao[idx], populacao[melhorIdx]);
        bool aceitaMesmoAssim = !venceu && (sorte(gen) < chanceAceitarPior);
        
        if(venceu || aceitaMesmoAssim)
            melhorIdx = idx;
        // if(aceitaMesmoAssim) break;
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
        while(pai2 == pai1 && tent < 2){
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
            
            bool herdaDoPai2 = zeroUm(gen) < 0.5;
            Individuo &paiHerdar = herdaDoPai2 ? pai2 : pai1;
            Individuo &otoPai    = herdaDoPai2 ? pai1 : pai2;

            // Começa herdando tudo do "outro pai"
            filho.atrAlunoParada = otoPai.atrAlunoParada;
            filho.atrAlunoRota   = otoPai.atrAlunoRota;


            // conta quantos alunos cada rota do paiHerdar tem
            vector<int> tamanhoRotaHerdar(quantidadeMaxRota, 0);
            for(int g = 0; g < gene; ++g) tamanhoRotaHerdar[paiHerdar.atrAlunoRota[g]]++;

            // só rotas não-vazias entram no sorteio
            vector<int> rotasCandidatas;
            for(int r = 0; r < quantidadeMaxRota; ++r){
                if(tamanhoRotaHerdar[r] > 0) rotasCandidatas.push_back(r);
            }

            int quantMax = min((int)rotasCandidatas.size(), max(1, (int)(quantidadeMaxRota * 0.75)));
            uniform_int_distribution<int> distQuant(1, max(1, quantMax));
            int quantidadeASerHerdada = distQuant(gen);

            // sorteio ponderado por tamanho: embaralha e ordena preferindo rotas maiores,
            // mas ainda com aleatoriedade (não é sempre "pega as N maiores")
            shuffle(rotasCandidatas.begin(), rotasCandidatas.end(), gen);
            stable_sort(rotasCandidatas.begin(), rotasCandidatas.end(),
                [&](int x, int y){ return tamanhoRotaHerdar[x] > tamanhoRotaHerdar[y]; });

            set<int> selecionadas(rotasCandidatas.begin(), rotasCandidatas.begin() + min((int)rotasCandidatas.size(), quantidadeASerHerdada));

            for(int g = 0; g < gene; ++g){
                if(selecionadas.count(paiHerdar.atrAlunoRota[g])){
                    filho.atrAlunoParada[g] = paiHerdar.atrAlunoParada[g];
                    filho.atrAlunoRota[g]   = paiHerdar.atrAlunoRota[g];
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
        
        uniform_real_distribution<double> muta(0.0, 1.0);
        normal_distribution<double> ruido(0.0, 0.1); 
        
        double mutacaoIntensa = 0.5;
        // cruzamento da intensidade de permutação
        for(int r = 0; r < quantidadeMaxRota; r++){
            double v1 = pai1.intensidadePermutaRota[r];
            double v2 = pai2.intensidadePermutaRota[r];

            double lo = min(v1, v2);
            double hi = max(v1, v2);
            double range = hi - lo;

            const double alphaBLX = 0.5; 

            uniform_real_distribution<double> distBlend(lo - alphaBLX * range, hi + alphaBLX * range);
            double filhoVal = distBlend(gen);

            auto ajustaIntensidade = [](double valor) -> double {
                if(valor > 1.0){
                    double excesso = valor - 1.0;
                    valor = 0.5 + excesso;
                } else if(valor < 0.0){
                    double excesso = -valor;
                    valor = 0.5 - excesso;
                }

                return clamp(valor, 0.0, 1.0);
            };

            filho.intensidadePermutaRota[r] = ajustaIntensidade(filhoVal);

            // cout << "\t " << filho.intensidadePermutaRota[r] << "   ";

            // mutação
            if(muta(gen) < mutacaoIntensa){ 
                filho.intensidadePermutaRota[r] = ajustaIntensidade(filho.intensidadePermutaRota[r] + ruido(gen));
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
        int idxRo = selecionaTorneio(resto, 0.3);
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


double desvio(const vector<double>& valores) {
    if (valores.empty())
        return 0.0;

    double soma = 0.0;

    for (double x : valores)
        soma += x;

    double media = soma / valores.size();

    double somaQuadrados = 0.0;

    for (double x : valores)
        somaQuadrados += pow(x - media, 2);

    double variancia = somaQuadrados / valores.size();

    return sqrt(variancia);
}


Individuo Metaheuristica::AG(int opc){

    int maxGeracao = 200;
    int tamanhoPopulacao = 100;
    double crossoverProb = 0.85; 
    double mutacaoProb = 0.01; // baixa mutacao é melhor
    int elitismo = max(1, int(tamanhoPopulacao * 0.01)); // pelo menos 1 // elitismo mais baixo émellhr
    // int elitismo = 1; // pelo menos 1 // elitismo mais baixo émellhr
    
    vector<Individuo> populacao = popIni(tamanhoPopulacao);
    
    vector<double> fitnessInit(tamanhoPopulacao);
    
    if(opc){
        for(int i = 0; i < tamanhoPopulacao; i++){
            fitnessInit[i] = buscaTabu(populacao[i]);
            aplicaPenalidades(populacao[i]);
        } 
    } else {
        for(int i = 0; i < tamanhoPopulacao; i++){
            vector<bool> sucesso;
            vector<vector<int>> rotasTemp = caminhosIniciais(populacao[i], sucesso);
            finalizaSolucao(populacao[i], rotasTemp, sucesso);
            aplicaPenalidades(populacao[i]);
            fitnessInit[i] = populacao[i].fitness;
        } 
    }

    int idxInit = 0;
    for(int i = 1; i < tamanhoPopulacao; ++i)
        if(maisViavel(populacao[i], populacao[idxInit])) idxInit = i;
    
    Individuo melhorIndividuo = populacao[idxInit];
    double melhorFitness = fitnessInit[idxInit];
    double estagnado = 0;
    
    // max iter e estagnação
    
    vector<double> valores;

    for(int i = 0; i < maxGeracao; ++i){

        vector<pair<int,int>> paisEscolhidos = escolhendoPais(populacao);
        
        vector<Individuo> filhos = reproducao(paisEscolhidos, populacao, tamanhoPopulacao, mutacaoProb, crossoverProb);

        // for (auto& filho : filhos) buscaTabu(filho); // fit
        
        if(opc){
            for (auto& filho : filhos){
                buscaTabu(filho);
                aplicaPenalidades(filho);
            }
        } else {
            for (auto& filho : filhos){
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(filho, sucesso);
                finalizaSolucao(filho, rotasTemp, sucesso);
                aplicaPenalidades(filho);
            } 
        }
        
        // ja chega ordenado
        vector<Individuo> novaPopulacao = novaPopTorneioElitista(filhos, populacao, tamanhoPopulacao, elitismo);

        
        // parametros de estagnação da mutação
        const double mutacaoBase = 0.01;
        const double mutacaoMax  = 0.98;
        const int limiarEstagnacao = 5;
        
        // procura por melhor individuo
        if (maisViavel(novaPopulacao[0], melhorIndividuo)){
            melhorIndividuo = novaPopulacao[0];
            melhorFitness = novaPopulacao[0].fitness;
            estagnado = i;
            mutacaoProb = mutacaoBase; 
        }

        // quantidade de gerações estagnadas
        int geracoesSemMelhora = i - estagnado;

        // aumento da mutação por estagnação
        if(geracoesSemMelhora > limiarEstagnacao){
            double excesso = geracoesSemMelhora - limiarEstagnacao;
            double incremento = 0.001 * (1.0 + excesso * 0.05); // cresce com o tempo estagnado
            mutacaoProb = clamp(mutacaoProb + incremento, mutacaoBase, mutacaoMax);
        }

        // injeção de individuos na estagnação
        int baseInjecao  = max(1, (int)(tamanhoPopulacao * 0.01));
        int extraInjecao = (int)(geracoesSemMelhora * 0.001);
        int quantInjetar = min((int)(tamanhoPopulacao * 0.20), baseInjecao + extraInjecao);
        
        for(int k = 0; k < quantInjetar; ++k){
            int idx = tamanhoPopulacao - 1 - k;
            if(idx < elitismo) break;

            Individuo novo = geraSolucaoInicial();
            if(opc) buscaTabu(novo);
            else {
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(novo, sucesso);
                finalizaSolucao(novo, rotasTemp, sucesso);
            }
            novaPopulacao[idx] = move(novo);
        }

        valores.push_back(novaPopulacao[0].fitness);

        populacao.swap(novaPopulacao);
        
        cout << novaPopulacao[0].fitness << " | "; 
        // fflush(stdout);
        
        if ( (i % 15) == 0){
            cout << "\n";
            // cout << "Geracao:" << i << " // melhor fitness atual = " << melhorFitness << "\n";
        }
    }
    
    cout << "\n";
    double dp = desvio(valores);
    double media = accumulate(valores.begin(), valores.end(), 0) / valores.size();

    cout << "Melhor valor: " << melhorFitness << "\n";
    cout << "Desvio padrao: " << dp << "\n";
    cout << "Media: " << media << "\n";
    
    compactarRotas(melhorIndividuo); 

    return melhorIndividuo;
}
