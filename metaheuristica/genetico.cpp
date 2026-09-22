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
    vector<vector<int>> paradasPorRota(quantidadeMaxRota); // paradas já usadas em cada rota

    for(const auto& aluno : problema.alunosParadas){

        vector<pair<int, double>> aux = aluno.paradasPossiveis;

        sort(aux.begin(), aux.end(), [](const pair<int, double>& a, const pair<int, double>& b){
            return a.second < b.second;
        });

        vector<double> pesosParada(aux.size());
        for(int i = 0; i < (int)aux.size(); ++i){
            pesosParada[i] = exp(-TaxaDecaimentoSolucaoInicial * i);
        }

        discrete_distribution<int> distPesoParada(pesosParada.begin(), pesosParada.end());

        int parada = aux[distPesoParada(gen)].first;

        fulano.atrAlunoParada[aluno.id] = parada;

        // Procura rotas que ainda têm capacidade
        vector<int> candidatas;
        for(int r = 0; r < quantidadeMaxRota; r++){
            if(cargaRota[r] < problema.Q)
                candidatas.push_back(r);
        }

        int rota;

        if(candidatas.empty()){
            rota = 0;
            for(int r = 1; r < quantidadeMaxRota; r++)
                if(cargaRota[r] < cargaRota[rota]) rota = r;
            fulano.alunosInviaveisQuant++;
        } else {
            // distância média da parada do aluno
            // até as paradas já presentes em cada rota candidata
            vector<pair<int,double>> rotaDist; // (índice da rota candidata, distância média)
            rotaDist.reserve(candidatas.size());

            for(int r : candidatas){
                if(paradasPorRota[r].empty()){
                    // rota ainda vazia: usa distância até o nó 0 como base =
                    double d = problema.grafoParadas[parada][0];
                    if(d == 0) d = 1e9; // sem aresta direta ao depósito: trata como distante
                    rotaDist.push_back({r, d});
                    continue;
                }

                double soma = 0.0;
                int contados = 0;
                for(int p : paradasPorRota[r]){
                    double d = problema.grafoParadas[parada][p];
                    if(d == 0) d = 1e9; // sem aresta direta: penaliza fortemente, não ignora
                    soma += d;
                    contados++;
                }
                rotaDist.push_back({r, soma / contados});
            }

            sort(rotaDist.begin(), rotaDist.end(), [](const pair<int,double>& a, const pair<int,double>& b){
                return a.second < b.second;
            });

            vector<double> pesosRota(rotaDist.size());
            for(int i = 0; i < (int)rotaDist.size(); ++i){
                pesosRota[i] = exp(-TaxaDecaimentoRotaInicial * i); 
            }

            discrete_distribution<int> distPesoRota(pesosRota.begin(), pesosRota.end());
            rota = rotaDist[distPesoRota(gen)].first;
        }

        fulano.atrAlunoRota[aluno.id] = rota;
        cargaRota[rota]++;
        paradasPorRota[rota].push_back(parada);
    }

    fulano.alunoPorRota = cargaRota;

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

    int rotasAtivas = 0;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(individuo.rotaViavel[r] && individuo.alunoPorRota[r] > 0){
            rotasAtivas++;
            if(individuo.alunoPorRota[r] <= LimiarParadasPorRotas){
                penalidade += PenalidadeRotaExtraPequena;
            }
        }
    }
    penalidade += rotasAtivas * PenalidadePorRotaAtiva;

    // --- penalidade por parada repetida entre rotas ---

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
            penalidade += (quant - limiteParadaPorRota) * PenalidadePorRepeticaoParada;
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

    return a.fitness < b.fitness;
}


int Metaheuristica::selecionaTorneio(vector<Individuo> &populacao){
    int n = populacao.size();
    int k = max(1, (int)(n * 0.02));

    uniform_int_distribution<int> torneio(0, n-1);

    vector<int> participantes(k);
    for(int i = 0; i < k; ++i){
        participantes[i] = torneio(gen);
    }

    sort(participantes.begin(), participantes.end(), [&](int a, int b){
        return maisViavel(populacao[a], populacao[b]);
    });

    vector<double> pesos(k);
    for(int i = 0; i < k; ++i){
        pesos[i] = exp(-TaxaDecaimentoTorneio * i); 
    }

    discrete_distribution<int> distPeso(pesos.begin(), pesos.end());
    return participantes[distPeso(gen)];
}

vector<pair<int,int>> Metaheuristica::escolhendoPais(vector<Individuo> &populacao){
    
    vector<pair<int,int>> paisEscolhidos;
    paisEscolhidos.reserve(TamanhoDaPopulacao);

    for(int k = 0; k < TamanhoDaPopulacao; ++k){
        int pai1 = selecionaTorneio(populacao);
        int pai2 = selecionaTorneio(populacao);
        int tent = 0;
        while(pai2 == pai1 && tent < 2){
            pai2 = selecionaTorneio(populacao);
            ++tent;
        }
        paisEscolhidos.emplace_back(pai1, pai2);  
    }
    return paisEscolhidos;
}

vector<Individuo> Metaheuristica::reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao){
    vector<Individuo> filhos;
    filhos.reserve(TamanhoDaPopulacao);

    int gene = populacao[0].atrAlunoParada.size(); // quantidade de alunos

    uniform_int_distribution<int> corte(0, max(1, gene - 1));

    uniform_real_distribution<double> zeroUm(0.0, 1.0);

    for (int i = 0; i < TamanhoDaPopulacao; ++i){
        int idxPai1 = paisEscolhidos[i].first;
        int idxPai2 = paisEscolhidos[i].second;

        Individuo &pai1 = populacao[idxPai1];
        Individuo &pai2 = populacao[idxPai2];

        Individuo filho; // parada e rota devem ser herdadas do mesmo pai, caso contrário fica quebrado
        filho.atrAlunoParada.resize(gene);
        filho.atrAlunoRota.resize(gene);
        filho.intensidadePermutaRota.resize(quantidadeMaxRota);
        
        // UX - Uniform Crossover
        if (zeroUm(gen) < ProbabilidadeCrossover){

            bool usaCrossoverPorRota = zeroUm(gen) < ProbabilidadeCrossoverBloco; // 50% bloco de rota, 50% gene a gene

            if(usaCrossoverPorRota){

                bool herdaDoPai2 = zeroUm(gen) < 0.5;
                Individuo &paiHerdar = herdaDoPai2 ? pai2 : pai1;
                Individuo &otoPai    = herdaDoPai2 ? pai1 : pai2;

                // Começa herdando tudo do "outro pai"
                filho.atrAlunoParada = otoPai.atrAlunoParada;
                filho.atrAlunoRota   = otoPai.atrAlunoRota;

                vector<int> tamanhoRotaHerdar(quantidadeMaxRota, 0);
                for(int g = 0; g < gene; ++g) tamanhoRotaHerdar[paiHerdar.atrAlunoRota[g]]++;

                vector<int> rotasCandidatas;
                for(int r = 0; r < quantidadeMaxRota; ++r){
                    if(tamanhoRotaHerdar[r] > 0) rotasCandidatas.push_back(r);
                }

                int quantMax = min((int)rotasCandidatas.size(), max(1, (int)(quantidadeMaxRota * 0.5)));
                uniform_int_distribution<int> distQuant(1, max(1, quantMax));
                int quantidadeASerHerdada = distQuant(gen);

                sort(rotasCandidatas.begin(), rotasCandidatas.end(),
                    [&](int x, int y){ return tamanhoRotaHerdar[x] > tamanhoRotaHerdar[y]; });

                // const double decaimento = 0.2;
                // const double alpha = 0.5; 
                set<int> selecionadas;
                vector<int> disponiveis = rotasCandidatas;

                while((int)selecionadas.size() < quantidadeASerHerdada && !disponiveis.empty()){
                    vector<double> pesos(disponiveis.size());
                    for(int i = 0; i < (int)disponiveis.size(); ++i){
                        // double pesoRank = exp(-decaimento * i);
                        // pesos[i] = alpha + (1.0 - alpha) * pesoRank;
                        pesos[i] = exp(-TaxaDecaimentoRotasRep * i);
                    }

                    discrete_distribution<int> distPeso(pesos.begin(), pesos.end());
                    int idxEscolhido = distPeso(gen);

                    selecionadas.insert(disponiveis[idxEscolhido]);
                    disponiveis.erase(disponiveis.begin() + idxEscolhido);
                }

                for(int g = 0; g < gene; ++g){
                    if(selecionadas.count(paiHerdar.atrAlunoRota[g])){
                        filho.atrAlunoParada[g] = paiHerdar.atrAlunoParada[g];
                        filho.atrAlunoRota[g]   = paiHerdar.atrAlunoRota[g];
                    }
                }

            } else {
                // uniform crossover: 50/50 por gene, sem noção de rota/bloco
                for(int c = 0; c < gene; c++){
                    if(zeroUm(gen) < 0.5){
                        filho.atrAlunoParada[c] = pai1.atrAlunoParada[c];
                        filho.atrAlunoRota[c]   = pai1.atrAlunoRota[c];
                    } else {
                        filho.atrAlunoParada[c] = pai2.atrAlunoParada[c];
                        filho.atrAlunoRota[c]   = pai2.atrAlunoRota[c];
                    }
                }
            }

        } else {
            // sem crossover: copia aleatoriamente um dos pais
            if (zeroUm(gen) < 0.5) filho = pai1;
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
            if(muta(gen) < ProbabilidadeMutacao){
                const estudante& aluno = problema.alunosParadas[g];

                if(!aluno.paradasPossiveis.empty()){
                    uniform_int_distribution<int> distParada(0, aluno.paradasPossiveis.size() - 1);
                    filho.atrAlunoParada[g] = aluno.paradasPossiveis[distParada(gen)].first;
                }
            }

            // Mutação da rota
            if(muta(gen) < ProbabilidadeMutacao){
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

vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais){

    vector<Individuo> combinado = filhos;
    combinado.insert(combinado.end(), pais.begin(), pais.end());

    sort(combinado.begin(), combinado.end(), [this](const Individuo &a, const Individuo &b){
        return maisViavel(a, b);
    });

    vector<Individuo> novaPop(combinado.begin(), combinado.begin() + Elitismo);
    vector<Individuo> resto(combinado.begin() + Elitismo, combinado.end());

    while(novaPop.size() < (size_t)TamanhoDaPopulacao){
        int idxRo = selecionaTorneio(resto);
        novaPop.push_back(resto[idxRo]);
    }

    sort(novaPop.begin(), novaPop.end(), [](Individuo a, Individuo b){
        return a.fitness < b.fitness;
    });

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
    
    vector<Individuo> populacao = popIni(TamanhoDaPopulacao);
    
    vector<double> fitnessPop(TamanhoDaPopulacao);
    
    if(opc){
        for(int i = 0; i < TamanhoDaPopulacao; i++){
            fitnessPop[i] = buscaTabu(populacao[i]);
            aplicaPenalidades(populacao[i]);
        } 
    } else {
        for(int i = 0; i < TamanhoDaPopulacao; i++){
            vector<bool> sucesso;
            vector<vector<int>> rotasTemp = caminhosIniciais(populacao[i], sucesso);
            finalizaSolucao(populacao[i], rotasTemp, sucesso);
            aplicaPenalidades(populacao[i]);
            fitnessPop[i] = populacao[i].fitness;
        } 
    }

    int idxInit = 0;
    for(int i = 1; i < TamanhoDaPopulacao; ++i)
        if(maisViavel(populacao[i], populacao[idxInit])) idxInit = i;
    
    Individuo melhorIndividuo = populacao[idxInit];
    double melhorFitness = fitnessPop[idxInit];
    double estagnado = 0;
    
    // max iter e estagnação
    
    vector<double> valores;
    
    for(int i = 0; i < numeroMaxGeracoes; ++i){
        
        double temperaturaSelecao = 0.4; 

        vector<pair<int,int>> paisEscolhidos = escolhendoPais(populacao);
        
        vector<Individuo> filhos = reproducao(paisEscolhidos, populacao);

        // for (auto& filho : filhos) buscaTabu(filho); // fit
        
        if(opc){
            int itera = 0;
            for (auto& filho : filhos){
                buscaTabu(filho);
                aplicaPenalidades(filho);
                fitnessPop[itera] = filho.fitness;
                ++itera;
            }
        } else {
            int itera = 0;
            for (auto& filho : filhos){
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(filho, sucesso);
                finalizaSolucao(filho, rotasTemp, sucesso);
                aplicaPenalidades(filho);
                fitnessPop[itera] = filho.fitness;
                ++itera;
            } 
        }
        
        // ja chega ordenado
        vector<Individuo> novaPopulacao = novaPopTorneioElitista(filhos, populacao);

        // quantidade de gerações estagnadas
        int geracoesSemMelhora = i - estagnado;

        // aumento da mutação por estagnação
        if(geracoesSemMelhora > limiarEstagnacaoMutacao){
            double excesso = geracoesSemMelhora - limiarEstagnacaoMutacao;
            double incremento = 0.001 * (1.0 + excesso * 0.05); // cresce com o tempo estagnado
            ProbabilidadeMutacao = clamp(ProbabilidadeMutacao + incremento, PisoTaxaMutacao, TetoTaxaMutacao);
        }

        // injeção de individuos na estagnação
        int baseInjecao  = max(1, (int)(TamanhoDaPopulacao * PorcentagemBaseInjecao));
        int extraInjecao = (int)(geracoesSemMelhora * PorcentagemExtraInjecao);
        int quantInjetar = min((int)(TamanhoDaPopulacao * PorcentagemMaximaNovosIndividuos), baseInjecao + extraInjecao);
        
        for(int k = 0; k < quantInjetar; ++k){
            int idx = TamanhoDaPopulacao - 1 - k;
            if(idx < Elitismo) break;

            Individuo novo = geraSolucaoInicial();
            if(opc){
                buscaTabu(novo);
                aplicaPenalidades(novo);
                fitnessPop[idx] = novo.fitness;
            } 
            else {
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(novo, sucesso);
                finalizaSolucao(novo, rotasTemp, sucesso);
                aplicaPenalidades(novo);
                fitnessPop[idx] = novo.fitness;
            }
            novaPopulacao[idx] = move(novo);
        }


        // procura por melhor individuo
        
        sort(novaPopulacao.begin(), novaPopulacao.end(), [this](const Individuo& a, const Individuo& b){
            return maisViavel(a,b);
        });


        if (maisViavel(novaPopulacao[0], melhorIndividuo)){
            melhorIndividuo = novaPopulacao[0];
            melhorFitness = novaPopulacao[0].fitness;
            estagnado = i;
            ProbabilidadeMutacao = PisoTaxaMutacao; 
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
    
    cout << "\n\n";
    double dp = desvio(valores);
    double media = accumulate(valores.begin(), valores.end(), 0) / valores.size();

    cout << "Melhor valor: " << melhorFitness << "\n";
    cout << "Desvio padrao: " << dp << "\n";
    cout << "Media: " << media << "\n";
    
    compactarRotas(melhorIndividuo); 

    return melhorIndividuo;
}
