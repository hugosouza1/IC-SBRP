#include "metaheuristica.hpp"


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
            // pesosParada[i] = exp(-TaxaDecaimentoSolucaoInicial * i);
            pesosParada[i] = exp(-TaxaDecaimentoSolucaoInicial * i) * pesoFreq(aux[i].first);
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

    // uniform_real_distribution<double> shake(0.3, 0.7);
    // fulano.intensidadePermutaRota.resize(quantidadeMaxRota);
    // for(int i = 0; i < quantidadeMaxRota; ++i){
    //     fulano.intensidadePermutaRota[i] = shake(gen);
    // }


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

    {
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

    }

    // --- penalidade de estouro de capacidade ---
    int estouro = 0, contador = 0;
    for(int i = 0; i < individuo.rotasFeitas.size(); ++i){
        estouro = individuo.alunoPorRota[i] - problema.capacidadeOnibus[individuo.atrOnibusRota[i]];
        if(estouro > 0){
            contador += estouro;
        }
    }

    penalidade += PenalidadeEstouroCapacidade * contador; 
        
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

void Metaheuristica::crossoverRotas(const Individuo& A, const Individuo& B, Individuo& filho){
    int n = problema.quantidadeAlunos;
    int R = quantidadeMaxRota;
    int P = problema.quantidadeParadas;
    uniform_real_distribution<double> zeroUm(0.0, 1.0);

    filho.atrAlunoParada.assign(n, -1);
    filho.atrAlunoRota.assign(n, -1);
    filho.atrOnibusRota = A.atrOnibusRota; // facilitar na parte do remanejo
    filho.atrOnibusRota.resize(R);

    vector<int> carga(R, 0);
    vector<char> usada(R, 0);
    vector<vector<int>> alunosA(R), alunosB(R);

    // mapeia as paradas de cada rota
    for(int g = 0; g < n; ++g){
        alunosA[A.atrAlunoRota[g]].push_back(g);
        alunosB[B.atrAlunoRota[g]].push_back(g);
    }

    auto copiaRota = [&](const Individuo& pai, int rPai, const vector<int>& alunos, int rFilho){
        filho.atrOnibusRota[rFilho] = pai.atrOnibusRota[rPai];

        usada[rFilho] = 1;
        carga[rFilho] = alunos.size();

        for(int g : alunos){
            filho.atrAlunoRota[g]   = rFilho;
            filho.atrAlunoParada[g] = pai.atrAlunoParada[g];
        }
    };

    // 1) k rotas de A (viés para as maiores), copiadas como unidade
    vector<int> cand;
    for(int r = 0; r < R; ++r) 
        if(!alunosA[r].empty()) cand.push_back(r);

    sort(cand.begin(), cand.end(), [&](int x, int y){ return alunosA[x].size() > alunosA[y].size(); });

    int kMax = max(1, (int)cand.size() / 2);
    int k = uniform_int_distribution<int>(1, kMax)(gen);

    while(k-- > 0 && !cand.empty()){
        vector<double> w(cand.size());
        for(int i = 0; i < (int)cand.size(); ++i) 
            w[i] = exp(-TaxaDecaimentoRotasRep * i); // maiores rotas tem mais chances de serem selecionadas

        int i = discrete_distribution<int>(w.begin(), w.end())(gen);
        int r = cand[i];
        
        cand.erase(cand.begin() + i);
        copiaRota(A, r, alunosA[r], r);
    }

    // 2) rotas de B que não colidem com nada já herdado
    vector<int> ordemB(R);
    iota(ordemB.begin(), ordemB.end(), 0);
    shuffle(ordemB.begin(), ordemB.end(), gen);

    for(int rb : ordemB){
        if(alunosB[rb].empty()) continue;

        bool livre = true;
        for(int g : alunosB[rb]) 
            if(filho.atrAlunoRota[g] >= 0) // filho n atribuido
                { livre = false; break; }

        if(!livre) continue;

        int rf = -1;
        for(int r = 0; r < R; ++r) if(!usada[r]){ rf = r; break; }

        if(rf < 0) break; // sem rótulo livre

        copiaRota(B, rb, alunosB[rb], rf);
    }

    // 3) reinserção dos alunos que sobraram
    vector<vector<char>> presente(R, vector<char>(P, 0)); // para mapear o custo de inserção
    vector<vector<int>> paradas(R);
    
    // ver quais rotas e paradas ja foram atribuidas
    for(int g = 0; g < n; ++g){
        int r = filho.atrAlunoRota[g];
        if(r < 0) continue;

        int p = filho.atrAlunoParada[g];
        if(!presente[r][p])
            { presente[r][p] = 1; paradas[r].push_back(p); }
    }

    vector<int> pend;
    for(int g = 0; g < n; ++g) 
        if(filho.atrAlunoRota[g] < 0) 
            pend.push_back(g);

    shuffle(pend.begin(), pend.end(), gen);

    uniform_real_distribution<double> ruido(0.9, 1.1);

    for(int g : pend){
        const auto& poss = problema.alunosParadas[g].paradasPossiveis;
        double melhor = INFINITO;
        int bp = -1, br = -1;

        for(int r = 0; r < R; ++r){
            if(carga[r] >= problema.capacidadeOnibus[filho.atrOnibusRota[r]]) continue;

            for(const auto& par : poss){
                int p = par.first;
                double c;

                if(presente[r][p]) 
                    c = 0.0;                                     // parada já aberta: custo zero
                else {
                    double dmin = distancias[0][p];              // proxy do custo de inserção
                    for(int q : paradas[r]) 
                        dmin = min(dmin, distancias[p][q]);
                    c = 2.0 * dmin * ruido(gen);
                }

                if(c < melhor)
                    { melhor = c; bp = p; br = r; }
            }
        }

        if(br < 0){ // tudo cheio: joga na rota mais vazia (a penalidade de capacidade cobra)
            br = 0;
            for(int r = 1; r < R; ++r) 
                if(carga[r] < carga[br]) 
                    br = r;

            bp = poss[0].first;
            for(const auto& par : poss) 
                if(par.second < poss[0].second) 
                bp = par.first;
        }

        filho.atrAlunoRota[g] = br;
        filho.atrAlunoParada[g] = bp;
        carga[br]++;
        usada[br] = 1;
        if(!presente[br][bp]){ presente[br][bp] = 1; paradas[br].push_back(bp); }
    }

    // mutação de ônibus, só se a capacidade nova comportar a rota
    uniform_int_distribution<int> distOnibus(0, problema.quantidadeOnibus - 1);
    for(int r = 0; r < R; ++r){
        if(zeroUm(gen) < ProbabilidadeMutacaoOnibus){
            int b = distOnibus(gen);
            if(problema.capacidadeOnibus[b] >= carga[r]) filho.atrOnibusRota[r] = b;
        }
    }
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

        Individuo filho;

        if (zeroUm(gen) < ProbabilidadeCrossover){
            crossoverRotas(pai1, pai2, filho);
        } else {
            filho = (zeroUm(gen) < 0.5) ? pai1 : pai2;
        }

        mutacaoIndividuo(filho);
        filhos.push_back(move(filho)); 
    }
    return filhos;
}



void Metaheuristica::mutacaoIndividuo(Individuo &filho, bool forcarMacro){
    int gene = filho.atrAlunoParada.size();

    uniform_real_distribution<double> muta(0.0, 1.0);
    normal_distribution<double> ruido(0.0, 0.1); 
    
    // double mutacaoIntensa = 0.5;
    // // cruzamento da intensidade de permutação
    // for(int r = 0; r < quantidadeMaxRota; r++){
    //     double v1 = intensidadePermutacao1[r];
    //     double v2 = intensidadePermutacao2[r];

    //     double lo = min(v1, v2);
    //     double hi = max(v1, v2);
    //     double range = hi - lo;

    //     const double alphaBLX = 0.5; 

    //     uniform_real_distribution<double> distBlend(lo - alphaBLX * range, hi + alphaBLX * range);
    //     double filhoVal = distBlend(gen);

    //     auto ajustaIntensidade = [](double valor) -> double {
    //         if(valor > 1.0){
    //             double excesso = valor - 1.0;
    //             valor = 0.5 + excesso;
    //         } else if(valor < 0.0){
    //             double excesso = -valor;
    //             valor = 0.5 - excesso;
    //         }

    //         return clamp(valor, 0.0, 1.0);
    //     };

    //     filho.intensidadePermutaRota[r] = ajustaIntensidade(filhoVal);

    //     // cout << "\t " << filho.intensidadePermutaRota[r] << "   ";

    //     // mutação
    //     if(muta(gen) < mutacaoIntensa){ 
    //         filho.intensidadePermutaRota[r] = ajustaIntensidade(filho.intensidadePermutaRota[r] + ruido(gen));
    //     }
    // }

    vector<int> cargaRota(quantidadeMaxRota, 0); 
    for(int g = 0; g < gene; ++g) cargaRota[filho.atrAlunoRota[g]]++;

    //  Mutação
    for(int g = 0; g < gene; ++g){
        // Mutação da parada
        if(muta(gen) < ProbabilidadeMutacao){
            const estudante& aluno = problema.alunosParadas[g];

            if(!aluno.paradasPossiveis.empty()){
                filho.atrAlunoParada[g] = sorteiaParada(aluno);
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
                // uniform_int_distribution<int> distRota(0, candidatas.size() - 1);
                // int novaRota = candidatas[distRota(gen)];
                int novaRota = sorteiaRota(candidatas, filho, filho.atrAlunoParada[g]);


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
                        filho.atrAlunoParada[g] = sorteiaParada(aluno);
                    }

                    // sorteia nova rota, com capacidade
                    vector<int> candidatas;
                    for(int r = 0; r < quantidadeMaxRota; ++r){
                        if(r != rotaAlvo && cargaRota[r] < problema.capacidadeOnibus[filho.atrOnibusRota[r]])
                            candidatas.push_back(r);
                    }

                    if(!candidatas.empty()){
                        // uniform_int_distribution<int> distR(0, candidatas.size() - 1);
                        // int novaRota = candidatas[distR(gen)];
                        int novaRota = sorteiaRota(candidatas, filho, filho.atrAlunoParada[g]);

                        cargaRota[rotaAlvo]--;
                        cargaRota[novaRota]++;
                        filho.atrAlunoRota[g] = novaRota;
                    }
                }
            }
        }
    }
}

 
int Metaheuristica::sorteiaRota(const vector<int>& candidatas, const Individuo& filho, int parada){
    vector<vector<int>> paradasNaRota(quantidadeMaxRota);
    vector<int> cargaRota(quantidadeMaxRota, 0);
    vector<vector<char>> presente(quantidadeMaxRota, vector<char>(problema.quantidadeParadas, 0));

    for(int i = 0; i < (int)filho.atrAlunoParada.size(); ++i){
        int r = filho.atrAlunoRota[i], p = filho.atrAlunoParada[i];
        cargaRota[r]++;
        if(!presente[r][p]){ presente[r][p] = 1; paradasNaRota[r].push_back(p); }
    }

    vector<double> pesos(candidatas.size());
    for(int i = 0; i < (int)candidatas.size(); ++i){
        int r = candidatas[i];
        double afinidade;

        if(paradasNaRota[r].empty()){
            afinidade = 0.05;
        } else {
            double dmin = INFINITO;
            for(int q : paradasNaRota[r]) dmin = min(dmin, distancias[parada][q]);
            afinidade = exp(-LambdaRota * min(dmin, 1e6) / distMediaEntreParadas);
        }

        double cap = problema.capacidadeOnibus[filho.atrOnibusRota[r]];
        pesos[i] = max(1e-9, afinidade * (1.0 + BetaCarga * cargaRota[r] / cap));
    }

    discrete_distribution<int> d(pesos.begin(), pesos.end());
    return candidatas[d(gen)];
}

double Metaheuristica::pesoFreq(int p){
    if(freqMaxParada <= 0) return 1.0;
    double fn = freqParada[p] / freqMaxParada;              // [0,1]
    return max(0.05, 1.0 + sinalFreqParada * LambdaFreq * fn); // piso evita peso <= 0
}

int Metaheuristica::sorteiaParada(const estudante& aluno){
    int k = aluno.paradasPossiveis.size();
    vector<double> pesos(k);
    for(int i = 0; i < k; ++i)
        pesos[i] = pesoFreq(aluno.paradasPossiveis[i].first);

    discrete_distribution<int> d(pesos.begin(), pesos.end());
    return aluno.paradasPossiveis[d(gen)].first;
}

void Metaheuristica::atualizaFrequencias(const vector<Individuo>& pop){
    for(double& f : freqParada) f *= (1.0 - DecaimentoFreq);

    int E = max(1, (int)(pop.size() * ElitePctFreq));
    for(int e = 0; e < E; ++e){
        unordered_set<int> usadas(pop[e].atrAlunoParada.begin(), pop[e].atrAlunoParada.end());
        for(int p : usadas) freqParada[p] += 1.0 / E;
    }
    freqMaxParada = *max_element(freqParada.begin(), freqParada.end());
}


// vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais){
//     vector<Individuo> combinado = filhos;
//     combinado.insert(combinado.end(), pais.begin(), pais.end());

//     vector<Individuo> nova = selecionaSobreviventes(combinado);
//     sort(nova.begin(), nova.end(), [](const Individuo& a, const Individuo& b){ return a.fitness < b.fitness; });
//     return nova;
// }

vector<Individuo> Metaheuristica::novaPopTorneioElitista(vector<Individuo>& filhos, vector<Individuo>& pais, int geracao){
    vector<Individuo> combinado = filhos, protegidos;
    for(auto& p : pais) (p.protegidoAte > geracao ? protegidos : combinado).push_back(p);

    int alvo = TamanhoDaPopulacao - (int)protegidos.size();
    vector<Individuo> nova = selecionaSobreviventes(combinado, alvo);
    nova.insert(nova.end(), protegidos.begin(), protegidos.end());

    sort(nova.begin(), nova.end(), [](const Individuo& a, const Individuo& b){ return a.fitness < b.fitness; });
    return nova;
}

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


void Metaheuristica::engavetamento(int i, vector<Individuo>& novaPopulacao){
    if(i % PeriodoArquivamento == 0){
        for(int k = 0; k < EngavetadosK; ++k){
            bool novo = true;
            for(const auto& g : Gaveta)
                if(distanciaIndividuos(novaPopulacao[k], g) < SemelhancaGaveta){ novo = false; break; }
            if(novo) Gaveta.push_back(novaPopulacao[k]);
        }
        while((int)Gaveta.size() > TamanhoGaveta) Gaveta.pop_front();
    }
}

void Metaheuristica::injecaoNovosIndividuos(int semMelhora, int i, vector<Individuo>& novaPopulacao, int& ultimaGaveta, int& ultimaImig){
    int InjecaoImig = max(1, (int)(TamanhoDaPopulacao * PorcentagemInjecaoImigrantes));
    int InjecaoGav  = max(1, (int)(TamanhoDaPopulacao * PorcentagemInjecaoGaveta));

    bool dispImig   = semMelhora > LimiarImigrantes && i - ultimaImig >= LimiarImigrantes;
    bool dispGaveta = semMelhora > LimiarGaveta     && i - ultimaGaveta >= LimiarGaveta && !Gaveta.empty();

    int slot = TamanhoDaPopulacao - 1;   // população já vem ordenada (pior no fim)

    if(dispImig){
        for(int k = 0; k < InjecaoImig && slot >= 1; ++k){   // protege o melhor
            Individuo novo = geraSolucaoInicial();
            buscaTabu(novo);
            aplicaPenalidades(novo);
            novo.protegidoAte = i + JanelaProtecao;
            novaPopulacao[slot--] = move(novo);
        }
        ultimaImig = i;
    }

    if(dispGaveta){
        int quant = min((int)Gaveta.size(), InjecaoGav);
        for(int k = 0; k < quant && slot >= 1; ++k){
            uniform_int_distribution<int> distGaveta(0, Gaveta.size() - 1);
            Individuo reinserido = Gaveta[distGaveta(gen)];

            mutacaoIndividuo(reinserido, true);
            buscaTabu(reinserido);
            aplicaPenalidades(reinserido);
            reinserido.protegidoAte = i + JanelaProtecao;
            novaPopulacao[slot--] = move(reinserido);
        }
        ultimaGaveta = i;
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



vector<Individuo> Metaheuristica::selecionaSobreviventes(vector<Individuo>& pool, int alvo){
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

    while(vivos > alvo){ // 1 pra substituir, varioa para a seleção da prox geracao

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
    sobreviventes.reserve(alvo);
    for(int i = 0; i < n; ++i) if(vivo[i]) sobreviventes.push_back(move(pool[i]));
    return sobreviventes;
}


pair<vector<double>, Individuo> Metaheuristica::AG(int opc){
    
    vector<Individuo> populacao = popIni(TamanhoDaPopulacao);
    
        for(int i = 0; i < TamanhoDaPopulacao; i++){
            buscaTabu(populacao[i]);
            aplicaPenalidades(populacao[i]);
        } 
    

    int idxInit = 0;
    for(int i = 1; i < TamanhoDaPopulacao; ++i)
        if(maisViavel(populacao[i], populacao[idxInit])) idxInit = i;
    
    Individuo melhorIndividuo = populacao[idxInit];
    vector<pair<int,double>> historico; // (geração, melhor fitnessPuro)
    
    double estagnado = 0;
    
    vector<double> valores;

    auto inicio = steady_clock::now();
    int TempoMaxSegundos = 120;


    int ultimoMelhorGlobal = 0, ultimaGaveta = 0, ultimaImig = 0;
    for(int i = 0; i < numeroMaxGeracoes; ++i){
        
        double temperaturaSelecao = 0.4; 

        vector<pair<int,int>> paisEscolhidos = escolhendoPais(populacao);
        
        vector<Individuo> filhos = reproducao(paisEscolhidos, populacao);


       int itera = 0;
            for (auto& filho : filhos){
                buscaTabu(filho);
                aplicaPenalidades(filho);
                ++itera;
            }
      
        
        // ja chega ordenado
        vector<Individuo> novaPopulacao = novaPopTorneioElitista(filhos, populacao, i);

        int semMelhora = i - ultimoMelhorGlobal;

        engavetamento(i, novaPopulacao);
        injecaoNovosIndividuos(semMelhora, i, novaPopulacao, ultimaGaveta, ultimaImig);

        // sort para prox pop
        sort(novaPopulacao.begin(), novaPopulacao.end(), [this](const Individuo& a, const Individuo& b){
            return maisViavel(a,b);
        });
        
        // sort pra salvar. ver com calma depois se o sort de cima precisa nas seleçõe de massa de probabilidade por exemplo 
        Individuo melhorDaGeracao = *min_element(novaPopulacao.begin(), novaPopulacao.end(),
            [](const Individuo& a, const Individuo& b){ return a.fitnessPuro < b.fitnessPuro;});

        // if (maisViavel(melhorDaGeracao, melhorIndividuo)){

        if (melhorDaGeracao.fitnessPuro + 1e-12 < melhorIndividuo.fitnessPuro){
            melhorIndividuo = melhorDaGeracao;
            historico.push_back({i, melhorIndividuo.fitnessPuro});
            ultimoMelhorGlobal = i;

        }

        atualizaFrequencias(novaPopulacao);

        sinalFreqParada = (semMelhora > LimiarFreqParadas) ? -1 : 1; // diversifica quando estagna'
        
        populacao.swap(novaPopulacao);

        double TickingAwayTheMomentsThatMakeUpADullDay = duration<double>(steady_clock::now() - inicio).count();

        if(log && i % 20 == 0){
            cout << "G [" << i << "] : ";
            cout << "time [" << TickingAwayTheMomentsThatMakeUpADullDay  << "s] : ";
            cout << "ultGaveta [" << ultimaGaveta << "] : ";
            cout << "ultImigrante [" << ultimaImig << "] : ";
            cout << "\n";
            cout << "populacao:\n";
            for(int imp = 0; imp < min(60, TamanhoDaPopulacao); ++imp){
                if(imp%10 == 0 && imp !=0) cout << "\n";
                cout << "" << populacao[imp].fitnessPuro << "(" << populacao[imp].penalidadeFitness << ")" <<  "\t";
                // else cout << populacao[imp].fitness << " | ";
            }

            cout << "\ngaveta[" << Gaveta.size() << "]:\n";
            for(int imp = 0; imp < min(60, (int)Gaveta.size()); ++imp){
                if(imp%10 == 0 && imp !=0) cout << "\n";
                cout << "" << Gaveta[imp].fitnessPuro << "(" << Gaveta[imp].penalidadeFitness << ")" <<  "\t";
            }

            cout << "\n\n";
        }
        if(TickingAwayTheMomentsThatMakeUpADullDay > TempoMaxSegundos) break;
    }
    
    if(log)
        for(auto [a, b]: historico){
            cout << "it: " << a << ", fit " << b << "\n";        
        }
    
    
    compactarRotas(melhorIndividuo); 
    
    if(!log)
        { cout << melhorIndividuo.fitnessPuro << "\n"; fflush(stdout);}
        
    return {valores, melhorIndividuo};
}
