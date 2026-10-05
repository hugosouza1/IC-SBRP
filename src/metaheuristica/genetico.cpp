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

        distanciaTotal += distancia(rotas[r]);
    }

    configParada.alunosInviaveisQuant = alunosAfetados;
    configParada.fitness = distanciaTotal;
    configParada.fitnessPuro = distanciaTotal;
}


Individuo Metaheuristica::geraSolucaoInicial(){
    // cout << "opa\n\n"; fflush(stdout);
    Individuo fulano;

    fulano.atrAlunoParada.resize(problema.quantidadeAlunos);
    fulano.atrAlunoRota.resize(problema.quantidadeAlunos);

    vector<int> cargaRota(quantidadeMaxRota, 0);
    vector<vector<int>> paradasPorRota(quantidadeMaxRota); // paradas já usadas em cada rota

    fulano.atrOnibusRota.assign(quantidadeMaxRota, 0);
    int rotasPorOnibus = quantidadeMaxRota / problema.quantidadeOnibus;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        fulano.atrOnibusRota[r] = min(r / rotasPorOnibus, problema.quantidadeOnibus - 1);
    }

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
            if(cargaRota[r] < problema.capacidadeOnibus[fulano.atrOnibusRota[r]])
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
        if(quant > LimiteParadaPorRota){
            penalidade += (quant - LimiteParadaPorRota) * PenalidadeParadaEntreRota;
        }
    }


    // --- penalidade por parada repetida na mesma rota
    unordered_set<int> contagemParada2;
    for(const auto& rota : individuo.rotasFeitas){
        for(int a : rota){
            auto ret = contagemParada2.insert(a);
            if( ! ret.second){
                penalidade += PenalidadeParadaMesmaRota;
            }
        }
        contagemParada2.clear();
    }


    // --- penalidade por total de paradas percorridas (todas as rotas)
    int totalParadas = 0;
    for(const auto& rota : individuo.rotasFeitas){
        for(int p : rota){
            if(p == 0) continue;
            totalParadas++;
        }
    }
    penalidade += totalParadas * PenalidadeParadasPercorridasGlobal;

        
    individuo.penalidadeFitness = penalidade;
    individuo.fitness = individuo.fitnessPuro + penalidade;

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
    int k = max(1, (int)(n * PorcentagemTorneioK));

    uniform_int_distribution<int> torneio(0, n-1);

    vector<int> participantes(k);
    for(int i = 0; i < k; ++i){
        participantes[i] = torneio(gen);
    }

    std::sort(participantes.begin(), participantes.end(), [&](int a, int b){
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
        filho.atrOnibusRota = pai1.atrOnibusRota;
        
        // Cruzamento + mutação do mapeamento rota->onibus
        filho.atrOnibusRota.resize(quantidadeMaxRota);
        uniform_int_distribution<int> distOnibus(0, problema.quantidadeOnibus - 1);
        for(int r = 0; r < quantidadeMaxRota; ++r){
            filho.atrOnibusRota[r] = (zeroUm(gen) < 0.5) ? pai1.atrOnibusRota[r] : pai2.atrOnibusRota[r];

            if(zeroUm(gen) < ProbabilidadeMutacaoOnibus){
                filho.atrOnibusRota[r] = distOnibus(gen);
            }
        }

        
        // Cruzamento das rotas
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

                std::sort(rotasCandidatas.begin(), rotasCandidatas.end(),
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
        
        // mutação
        mutacaoIndividuo(filho, pai1.intensidadePermutaRota, pai2.intensidadePermutaRota);
        
        
        //  Assegura um filho factivel. Remanejo de aluno
        vector<int> cargaRota(quantidadeMaxRota, 0); 
        for(int g = 0; g < gene; ++g) cargaRota[filho.atrAlunoRota[g]]++;
        for(int g = 0; g < gene; ++g){
            int rotaAtual = filho.atrAlunoRota[g];

            if(cargaRota[rotaAtual] > problema.capacidadeOnibus[filho.atrOnibusRota[rotaAtual]]){
                vector<int> candidatas;

                for(int r = 0; r < quantidadeMaxRota; ++r){
                    if(cargaRota[r] < problema.capacidadeOnibus[filho.atrOnibusRota[r]])
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
          


        filhos.push_back(move(filho)); 
    }
    return filhos;
}



void Metaheuristica::mutacaoIndividuo(Individuo &filho, vector<double> intensidadePermutacao1, vector<double> intensidadePermutacao2, bool forcarMacro){
    int gene = filho.atrAlunoParada.size();

    uniform_real_distribution<double> muta(0.0, 1.0);
    normal_distribution<double> ruido(0.0, 0.1); 
    
    double mutacaoIntensa = 0.5;
    // cruzamento da intensidade de permutação
    for(int r = 0; r < quantidadeMaxRota; r++){
        double v1 = intensidadePermutacao1[r];
        double v2 = intensidadePermutacao2[r];

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

    vector<int> cargaRota(quantidadeMaxRota, 0); 
    for(int g = 0; g < gene; ++g) cargaRota[filho.atrAlunoRota[g]]++;

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

                if(cargaRota[r] < problema.capacidadeOnibus[filho.atrOnibusRota[r]])
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


    if(forcarMacro || muta(gen) < ProbabilidadeMacroMutacao){

        vector<int> possi;
        for(int r = 0; r < quantidadeMaxRota; ++r){
            if(cargaRota[r] > 0) possi.push_back(r);
        }

        if(!possi.empty()){
            uniform_int_distribution<int> distRota(0, possi.size() - 1);
            int rotaAlvo = possi[distRota(gen)];

            for(int g = 0; g < gene; ++g){
                if(filho.atrAlunoRota[g] == rotaAlvo){

                    // sorteia nova parada dentre as possíveis do aluno
                    const estudante& aluno = problema.alunosParadas[g];
                    if(!aluno.paradasPossiveis.empty()){
                        uniform_int_distribution<int> distParada(0, aluno.paradasPossiveis.size() - 1);
                        filho.atrAlunoParada[g] = aluno.paradasPossiveis[distParada(gen)].first;
                    }

                    // sorteia nova rota, com capacidade
                    vector<int> candidatas;
                    for(int r = 0; r < quantidadeMaxRota; ++r){
                        if(r != rotaAlvo && cargaRota[r] < problema.capacidadeOnibus[filho.atrOnibusRota[r]])
                            candidatas.push_back(r);
                    }
                    if(!candidatas.empty()){
                        uniform_int_distribution<int> distR(0, candidatas.size() - 1);
                        int novaRota = candidatas[distR(gen)];
                        cargaRota[rotaAlvo]--;
                        cargaRota[novaRota]++;
                        filho.atrAlunoRota[g] = novaRota;
                    }
                }
            }
        }
    }
}



vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais){
    vector<Individuo> combinado = filhos;
    combinado.insert(combinado.end(), pais.begin(), pais.end());

    vector<Individuo> nova = selecionaSobreviventes(combinado);
    sort(nova.begin(), nova.end(), [](const Individuo& a, const Individuo& b){ return a.fitness < b.fitness; });
    return nova;
}

// vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais){

//     vector<Individuo> combinado = filhos;
//     combinado.insert(combinado.end(), pais.begin(), pais.end());

//     std::sort(combinado.begin(), combinado.end(), [this](const Individuo &a, const Individuo &b){
//         return maisViavel(a, b);
//     });

//     vector<Individuo> novaPop(combinado.begin(), combinado.begin() + Elitismo);
//     vector<Individuo> resto(combinado.begin() + Elitismo, combinado.end());

//     while(novaPop.size() < (size_t)TamanhoDaPopulacao){
//         int idxRo = selecionaTorneio(resto);
//         novaPop.push_back(resto[idxRo]);
//     }

//     std::sort(novaPop.begin(), novaPop.end(), [](Individuo a, Individuo b){
//         return a.fitness < b.fitness;
//     });

//     return novaPop;
// }


void compactarRotas(Individuo& individuo) {

    vector<vector<int>> novasRotas;
    vector<int> mapa(individuo.rotasFeitas.size(), -1);
    vector<int> novaOniRota;

    for(int i = 0; i < individuo.rotasFeitas.size(); ++i) {

        if(individuo.rotasFeitas[i].empty())
            continue;

        mapa[i] = novasRotas.size();
        novasRotas.push_back(move(individuo.rotasFeitas[i]));
        novaOniRota.push_back(individuo.atrOnibusRota[i]);
    }
    

    vector<int> alPRota(novasRotas.size(), 0);
    for(int e = 0; e < individuo.atrAlunoRota.size(); ++e) {

        int rota = individuo.atrAlunoRota[e];
        if(rota >= 0 && rota < mapa.size()) {
            individuo.atrAlunoRota[e] = mapa[rota];
            alPRota[mapa[rota]]++;
        }
    }


    
    individuo.atrOnibusRota = move(novaOniRota);
    individuo.alunoPorRota = move(alPRota);
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


void Metaheuristica::injecaoNovosIndividuos(int opc, int geracoesSemMelhora, int i, vector<Individuo> &novaPopulacao){
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
    quantInjetar = quantInjetar / 4;

    for(int k = 0; k < quantInjetar; ++k){
        int idx = TamanhoDaPopulacao - 1 - k;
        if(idx < Elitismo) break;

        Individuo novo = geraSolucaoInicial();
        if(opc){
            buscaTabu(novo);
        } 
        else {
            vector<bool> sucesso;
            vector<vector<int>> rotasTemp = caminhosIniciais(novo, sucesso);
            finalizaSolucao(novo, rotasTemp, sucesso);
        }

        aplicaPenalidades(novo);
        
        novaPopulacao[idx] = move(novo);
    }

        


    if(i % PeriodoArquivamento == 0){
        for(int k = 0; k < EngavetadosK; ++k){
            bool novo = true;
            for(const auto& g : Gaveta)
                if(distanciaIndividuos(novaPopulacao[k], g) < SemelhancaGaveta){ novo = false; break; }
            if(novo) Gaveta.push_back(novaPopulacao[k]);
        }
        while((int)Gaveta.size() > TamanhoGaveta) Gaveta.pop_front();
    }

    
    if(geracoesSemMelhora > LimiarGaveta && !Gaveta.empty()){
        int quant = min((int)Gaveta.size(), quantInjetar * 3 / 4);
        for(int k = 0; k < quant; ++k){
            uniform_int_distribution<int> distGaveta(0, Gaveta.size() - 1);
            Individuo reinserido = Gaveta[distGaveta(gen)];
            
            // perturba antes de reinserir (macro-mutação), depois reavalia se mexeu nos genes
            vector<double> intenso1(quantidadeMaxRota, 0.3); // gambiarra
            vector<double> intenso2(quantidadeMaxRota, 0.7);

            mutacaoIndividuo(reinserido, intenso1, intenso2, true);

            if(opc) buscaTabu(reinserido);
            else {
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(reinserido, sucesso);
                finalizaSolucao(reinserido, rotasTemp, sucesso);
            }

            aplicaPenalidades(reinserido);
            
            int idx = TamanhoDaPopulacao - 1 - k;
            if(idx < Elitismo) break;
            novaPopulacao[idx] = move(reinserido);
        }
    }
}

// similiaridade de Jaccard
double Metaheuristica::distanciaIndividuos(const Individuo& a, const Individuo& b){
    int gene = a.atrAlunoRota.size();

    int difParada = 0;
    for(int g = 0; g < gene; ++g)
        if(a.atrAlunoParada[g] != b.atrAlunoParada[g]) difParada++;
    double dParada = (double)difParada / gene;

    vector<long long> ca(quantidadeMaxRota, 0), cb(quantidadeMaxRota, 0); // quantidade por rota em "a" e "b"
    unordered_map<int, long long> cel;

    cel.reserve(gene);
    for(int g = 0; g < gene; ++g){
        int ra = a.atrAlunoRota[g], rb = b.atrAlunoRota[g];
        ca[ra]++; cb[rb]++;
        cel[ra * quantidadeMaxRota + rb]++;
    }

    auto pares = [](long long n){ return n * (n - 1) / 2; }; // quantidade de pares que pode se formar

    long long SA = 0, SB = 0, SAB = 0;
    for(int r = 0; r < quantidadeMaxRota; ++r){ SA += pares(ca[r]); SB += pares(cb[r]); }
    for(auto& [k, n] : cel) SAB += pares(n);

    long long uniao = SA + SB - SAB; 
    double dRota = (uniao == 0) ? 0.0 : 1.0 - (double)SAB / uniao;

    return PesoDistParada * dParada + (1.0 - PesoDistParada) * dRota;
}



vector<Individuo> Metaheuristica::selecionaSobreviventes(vector<Individuo>& pool){
    int n = pool.size();

    vector<vector<double>> D(n, vector<double>(n, 0.0));
    for(int i = 0; i < n; ++i)
        for(int j = i + 1; j < n; ++j)
            D[i][j] = D[j][i] = distanciaIndividuos(pool[i], pool[j]);

    int melhor = 0;
    for(int i = 1; i < n; ++i)
        if(pool[i].fitness < pool[melhor].fitness) melhor = i;

    vector<bool> vivo(n, true);
    int vivos = n;

    while(vivos > TamanhoDaPopulacao){ // 1 pra substituir, varioa para a seleção da prox geracao

        vector<int> idx;
        for(int i = 0; i < n; ++i) if(vivo[i]) idx.push_back(i);
        int m = idx.size();

        // 1) clones: sai o pior do par
        int vitima = -1;
        for(int a = 0; a < m && vitima < 0; ++a){
            for(int b = a + 1; b < m; ++b){
                if(D[idx[a]][idx[b]] < EpsClone){
                    int i = idx[a], j = idx[b];
                    vitima = (pool[i].fitness > pool[j].fitness) ? i : j;
                    break;
                }
            }
        }

        // 2) sem clones: pior fitness enviesado (rank fitness + rank diversidade)
        if(vitima < 0){
            vector<double> div(n, 0.0);
            for(int i : idx){ // diversidade em relação a k vizinhos
                vector<double> d;
                d.reserve(m - 1);
                for(int j : idx) if(j != i) d.push_back(D[i][j]);

                int k = min(NVizinhosDiv, (int)d.size());
                partial_sort(d.begin(), d.begin() + k, d.end());
                double s = 0.0;

                for(int t = 0; t < k; ++t) s += d[t];
                div[i] = s / k;
            }

            vector<int> porFit = idx, porDiv = idx;
            sort(porFit.begin(), porFit.end(), [&](int a, int b){ return pool[a].fitness < pool[b].fitness; });
            sort(porDiv.begin(), porDiv.end(), [&](int a, int b){ return div[a] > div[b]; });

            vector<double> rankFit(n, 0.0), rankDiv(n, 0.0);
            for(int r = 0; r < m; ++r){
                rankFit[porFit[r]] = (double)r / (m - 1);
                rankDiv[porDiv[r]] = (double)r / (m - 1);
            }

            double pior = -1.0;
            for(int i : idx){
                if(i == melhor) continue;
                double enviesado = rankFit[i] + PesoDiversidade * rankDiv[i];
                if(enviesado > pior){ pior = enviesado; vitima = i; }
            }
        }

        vivo[vitima] = false;
        vivos--;
    }

    vector<Individuo> sobreviventes;
    sobreviventes.reserve(TamanhoDaPopulacao);
    for(int i = 0; i < n; ++i) if(vivo[i]) sobreviventes.push_back(move(pool[i]));
    return sobreviventes;
}


pair<vector<double>, Individuo> Metaheuristica::AG(int opc){
    
    vector<Individuo> populacao = popIni(TamanhoDaPopulacao);
    
    if(opc){
        for(int i = 0; i < TamanhoDaPopulacao; i++){
            buscaTabu(populacao[i]);
            aplicaPenalidades(populacao[i]);
        } 
    } else {
        for(int i = 0; i < TamanhoDaPopulacao; i++){
            vector<bool> sucesso;
            vector<vector<int>> rotasTemp = caminhosIniciais(populacao[i], sucesso);
            finalizaSolucao(populacao[i], rotasTemp, sucesso);
            aplicaPenalidades(populacao[i]);
        } 
    }

    int idxInit = 0;
    for(int i = 1; i < TamanhoDaPopulacao; ++i)
        if(maisViavel(populacao[i], populacao[idxInit])) idxInit = i;
    
    Individuo melhorIndividuo = populacao[idxInit];
    double estagnado = 0;
    
    // max iter e estagnação
    
    vector<double> valores;
    
    for(int i = 0; i < numeroMaxGeracoes; ++i){
        
        double temperaturaSelecao = 0.4; 

        vector<pair<int,int>> paisEscolhidos = escolhendoPais(populacao);
        
        vector<Individuo> filhos = reproducao(paisEscolhidos, populacao);


        if(opc){
            int itera = 0;
            for (auto& filho : filhos){
                buscaTabu(filho);
                aplicaPenalidades(filho);
                ++itera;
            }
        } else {
            int itera = 0;
            for (auto& filho : filhos){
                vector<bool> sucesso;
                vector<vector<int>> rotasTemp = caminhosIniciais(filho, sucesso);
                finalizaSolucao(filho, rotasTemp, sucesso);
                aplicaPenalidades(filho);
                ++itera;
            } 
        }
        
        // ja chega ordenado
        vector<Individuo> novaPopulacao = novaPopTorneioElitista(filhos, populacao);

        // quantidade de gerações estagnadas
        int geracoesSemMelhora = i - estagnado;

        injecaoNovosIndividuos(opc, geracoesSemMelhora, i, novaPopulacao);

        // sort para prox pop
        sort(novaPopulacao.begin(), novaPopulacao.end(), [this](const Individuo& a, const Individuo& b){
            return maisViavel(a,b);
        });
        
        // sort pra salvar. ver com calma depois se o sort de cima precisa nas seleçõe de massa de probabilidade por exemplo 
        Individuo melhorDaGeracao = *min_element(novaPopulacao.begin(), novaPopulacao.end(),
            [](const Individuo& a, const Individuo& b){ return a.fitnessPuro < b.fitnessPuro;});

        // if (maisViavel(melhorDaGeracao, melhorIndividuo)){
        if (melhorDaGeracao.fitnessPuro <  melhorIndividuo.fitnessPuro ){
            melhorIndividuo = melhorDaGeracao;
            estagnado = i;
            ProbabilidadeMutacao = PisoTaxaMutacao; 
        }
        
        populacao.swap(novaPopulacao);
        
        cout << "G: " << i << " | ";
        for(int imp = 0; imp < min(15, TamanhoDaPopulacao); ++imp){
            // if(imp == 0)
             cout << populacao[imp].fitnessPuro << "(" << populacao[imp].penalidadeFitness << ")" <<  " | ";
            // else cout << populacao[imp].fitness << " | ";
        }
        cout << "\n\n";
        
        // valores.push_back(populacao[0].fitness);
        
        // if ( (i % 15) == 0){
            // cout << "\n";
            // cout << "Geracao:" << i << " // melhor fitness atual = " << melhorFitness << "\n";
        // }
    }
    
    // cout << "\n\n";
    
    
    compactarRotas(melhorIndividuo); 
    
    cout << melhorIndividuo.fitnessPuro << "\n"; fflush(stdout);
    // cout << "owvnwdon";
    
    
    return {valores, melhorIndividuo};
}
