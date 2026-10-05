#include "metaheuristica.hpp"
#include "../SBRP.hpp"

double desvioPadrao(const std::vector<double> &valores);
static ParametrosAlgoritmo parseParametros(int argc, char **argv);


int somenteTabu(Metaheuristica &meta){
    cout << "\nBT:\n";
        int geracion = 100;
        Individuo melhorInd;
        vector<double> valores;
        for(int i = 0; i < geracion; ++i){
            Individuo beltrano = meta.geraSolucaoInicial();   
            meta.buscaTabu(beltrano);
            if(beltrano.fitness < melhorInd.fitness){
                melhorInd = beltrano;
            }
            cout << beltrano.fitness << "\t";
            if(i%16==0){
                cout << "\n";
            }
            
            valores.push_back(beltrano.fitness);
        }
        
        double dp = desvioPadrao(valores);
        double media = accumulate(valores.begin(), valores.end(), 0);
        
        cout << "\nMelhor valor: " << melhorInd.fitness << "\n";
        cout << "Desvio Padrao: " << dp << "\nMedia: " << media / valores.size() << "\n";
}


int hibrido(Metaheuristica &meta, infoSBRP &dados){
    cout << "\nAG-BT:\n";

    auto [valoresGeracoes, ciclano]  = meta.AG(1);

    meta.imprimeSolucao(ciclano, dados, valoresGeracoes);

    return 0;
}


int main(int argc, char *argv[]){
    if(argc < 2){
        cerr << "ERRO: falta o arquivo de instancia\n";
        return 1;
    }

    infoSBRP dados(argv[1]);
    // dados.leitura(argv[1]);

    ParametrosAlgoritmo p = parseParametros(argc, argv);

    Metaheuristica meta(dados, p);

    hibrido(meta, dados);

    return 0;
}








ParametrosAlgoritmo parseParametros(int argc, char** argv) {

    ParametrosAlgoritmo p;

    for (int i = 2; i < argc; ++i) {

        string arg = argv[i];

        auto next = [&](const char* nome) -> string {
            if (i + 1 >= argc) {
                cerr << "Parametro ausente para " << nome << "\n";
                exit(1);
            }

            return argv[++i];
        };

        if(arg == "--seed") // coisa
            p.seed = stoi((next("--seed")));            
        // ============================================================
        // ALGORITMO GENÉTICO
        // ============================================================

        // Parâmetros gerais
        else if (arg == "--max_geracoes")
            p.numeroMaxGeracoes = stoi(next("--max_geracoes"));

        else if (arg == "--populacao")
            p.tamanhoDaPopulacao = stoi(next("--populacao"));

        else if (arg == "--crossover")
            p.probabilidadeCrossover = stod(next("--crossover"));

        else if (arg == "--elitismo")
            p.elitismo = stoi(next("--elitismo"));


        // Mutação
        else if (arg == "--mutacao")
            p.probabilidadeMutacao = stod(next("--mutacao"));

        else if (arg == "--piso_mutacao")
            p.pisoTaxaMutacao = stod(next("--piso_mutacao"));

        else if (arg == "--teto_mutacao")
            p.tetoTaxaMutacao = stod(next("--teto_mutacao"));

        else if (arg == "--limiar_estagnacao_mutacao")
            p.limiarEstagnacaoMutacao =
                stoi(next("--limiar_estagnacao_mutacao"));

        else if (arg == "--macro_mutacao")
            p.probabilidadeMacroMutacao =
                stod(next("--macro_mutacao"));


        // Injeção de indivíduos
        else if (arg == "--injecao_base")
            p.porcentagemBaseInjecao =
                stod(next("--injecao_base"));

        else if (arg == "--injecao_extra")
            p.porcentagemExtraInjecao =
                stod(next("--injecao_extra"));

        else if (arg == "--injecao_max")
            p.porcentagemMaximaNovosIndividuos =
                stod(next("--injecao_max"));


        // Gaveta
        else if (arg == "--tamanho_gaveta")
            p.tamanhoGaveta = stoi(next("--tamanho_gaveta"));

        else if (arg == "--periodo_arquivamento")
            p.periodoArquivamento =
                stoi(next("--periodo_arquivamento"));

        else if (arg == "--limiar_gaveta")
            p.limiarGaveta = stoi(next("--limiar_gaveta"));

        else if (arg == "--engavetados_k")
            p.engavetadosK = stoi(next("--engavetados_k"));

        else if (arg == "--semelhanca_gaveta")
            p.semelhancaGaveta =
                stod(next("--semelhanca_gaveta"));


        // Penalidades de rota
        else if (arg == "--penalidade_rota")
            p.penalidadePorRotaAtiva =
                stod(next("--penalidade_rota"));

        else if (arg == "--limiar_paradas_rota")
            p.limiarParadasPorRotas =
                stoi(next("--limiar_paradas_rota"));

        else if (arg == "--penalidade_rota_curta")
            p.penalidadeRotaExtraPequena =
                stod(next("--penalidade_rota_curta"));

        else if (arg == "--penalidade_desbalanceamento")
            p.penalidadeDesbalanceamento =
                stod(next("--penalidade_desbalanceamento"));
        
        else if (arg == "--PenalidadeParadasPercorridasGlobal")
            p.PenalidadeParadasPercorridasGlobal =
                stod(next("--PenalidadeParadasPercorridasGlobal"));


        // Penalidades de parada
        else if (arg == "--limite_parada_rota")
            p.limiteParadaPorRota =
                stoi(next("--limite_parada_rota"));

        else if (arg == "--penalidade_parada_entre")
            p.penalidadeParadaEntreRota =
                stod(next("--penalidade_parada_entre"));

        else if (arg == "--penalidade_parada_mesma")
            p.penalidadeParadaMesmaRota =
                stod(next("--penalidade_parada_mesma"));


        // Torneio
        else if (arg == "--torneio_k")
            p.porcentagemTorneioK =
                stod(next("--torneio_k"));

        else if (arg == "--decaimento_torneio")
            p.taxaDecaimentoTorneio =
                stod(next("--decaimento_torneio"));


        // Seleção
        else if (arg == "--peso_dist_parada")
            p.pesoDistParada =
                stod(next("--peso_dist_parada"));

        else if (arg == "--vizinhos_div")
            p.nVizinhosDiv =
                stoi(next("--vizinhos_div"));

        else if (arg == "--peso_diversidade")
            p.pesoDiversidade =
                stod(next("--peso_diversidade"));

        else if (arg == "--eps_clone")
            p.epsClone =
                stod(next("--eps_clone"));


        // Reprodução
        else if (arg == "--crossover_bloco")
            p.probabilidadeCrossoverBloco =
                stod(next("--crossover_bloco"));

        else if (arg == "--decaimento_rotas_rep")
            p.taxaDecaimentoRotasRep =
                stod(next("--decaimento_rotas_rep"));

        else if (arg == "--mutacao_onibus")
            p.probabilidadeMutacaoOnibus =
                stod(next("--mutacao_onibus"));


        // Solução inicial
        else if (arg == "--decaimento_solucao_inicial")
            p.taxaDecaimentoSolucaoInicial =
                stod(next("--decaimento_solucao_inicial"));

        else if (arg == "--decaimento_rota_inicial")
            p.taxaDecaimentoRotaInicial =
                stod(next("--decaimento_rota_inicial"));


        // ============================================================
        // BUSCA TABU
        // ============================================================

        else if (arg == "--rcl")
            p.tamanhoRCL =
                stoi(next("--rcl"));

        else if (arg == "--decaimento_rcl")
            p.taxaDecaimentoRCL =
                stod(next("--decaimento_rcl"));

        else if (arg == "--tenure_rota")
            p.tenureTaxaTamRota =
                stod(next("--tenure_rota"));

        else if (arg == "--iteracoes_tabu")
            p.iteracoesTabu =
                stoi(next("--iteracoes_tabu"));


        // Recompensas
        else if (arg == "--recompensa_novo_melhor")
            p.recompensaNovoMelhor =
                stod(next("--recompensa_novo_melhor"));

        else if (arg == "--recompensa_melhora")
            p.recompensaMelhora =
                stod(next("--recompensa_melhora"));

        else if (arg == "--recompensa_aceito")
            p.recompensaAceito =
                stod(next("--recompensa_aceito"));


        // Memória
        else if (arg == "--decaimento_peso")
            p.fatorDecaimentoPeso =
                stod(next("--decaimento_peso"));

        else if (arg == "--tamanho_segmento")
            p.tamanhoSegmento =
                stoi(next("--tamanho_segmento"));
    }

    return p;
}

    

















void Metaheuristica::imprimeSolucao(Individuo& sol, infoSBRP& dados, vector<double> valores){
    cout << "\n========================================\n";
    cout << "        SOLUCAO - AG-BT (SBRP)\n";
    cout << "========================================\n\n";
    cout << "Distancia total: " << sol.fitness - sol.penalidadeFitness << " [penalidade:" << sol.penalidadeFitness << "]" << "\n";
    cout << "Desvio Padrão: " << desvioPadrao(valores) << "\n";

    // agrupa os alunos por rota original (antes de renumerar)
    vector<vector<int>> alunosPorRota(sol.rotasFeitas.size());
    for(int aluno = 0; aluno < dados.quantidadeAlunos; aluno++){
        int rota = sol.atrAlunoRota[aluno];
        alunosPorRota[rota].push_back(aluno);
    }

    int rotaImpressa = 0;
    for(int r = 0; r < (int)sol.rotasFeitas.size(); r++){

        if(sol.rotasFeitas[r].empty()) continue;

        int pesoRota = 0;
        for(int i = 0; i + 1 < (int)sol.rotasFeitas[r].size(); i++){
            pesoRota += dados.grafoParadas[sol.rotasFeitas[r][i]][sol.rotasFeitas[r][i+1]];
        }

        cout << "\n\n---- Rota " << rotaImpressa << " ----\n";
        cout << "Ônibus atribuído: " << sol.atrOnibusRota[rotaImpressa] << "(capacidade: " << problema.capacidadeOnibus[sol.atrOnibusRota[rotaImpressa]] << ")\n";
        cout << "Peso da rota: " << pesoRota << "\n";

        cout << "Trajeto (paradas): ";
        for(int i = 0; i < (int)sol.rotasFeitas[r].size(); i++){
            cout << sol.rotasFeitas[r][i];
            if(i + 1 < (int)sol.rotasFeitas[r].size()) cout << " -> ";
        }
        cout << "\n";

        cout << "Estudantes (" << alunosPorRota[r].size() << "): ";
        int it = 0;
        cout << "[parada->alunos]\n";
        set<int> para;

        for(int aluno : alunosPorRota[r]){
            para.insert(sol.atrAlunoParada[aluno]);
        }
        
        for(int parad : para){
            cout << "\t\t[" << parad << " -> ";
            bool pri = true;
            for(int aluno : alunosPorRota[r]){
                if(parad == sol.atrAlunoParada[aluno]){
                    if(pri){   
                        cout << aluno << "";
                        pri = false;
                    }
                    else cout << ", " << aluno;

                }
                // cout << "[" << aluno << "->" << sol.atrAlunoParada[aluno] << "], ";
            }
            cout << "]\n";
        }

        rotaImpressa++;
    }

    cout << "\n----------------------------------------\n";
    cout << "Total de onibus usados: " << rotaImpressa << "\n";
    cout << "Distancia total: " << sol.fitness - sol.penalidadeFitness << " [penalidade:" << sol.penalidadeFitness << "]" << "\n";
    cout << "========================================\n";
}



// busca tabu
double desvioPadrao(const vector<double>& valores) {
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