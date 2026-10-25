#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#include <string>
#include <queue>
#include <random>
#include <set>
#include <utility>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <unistd.h>
#include <iomanip>
#include <climits>

#include <unordered_set>
#include <unordered_map>

#include <cstdlib>

#include "../SBRP.hpp"

#include "../distancias.hpp"

using namespace chrono;
using namespace std;

// Description of the best-so-far configuration:
//    .ID. populacao crossover crossover_bloco piso_mutacao teto_mutacao macro_mutacao injecao_base injecao_max torneio_k decaimento_torneio peso_diversidade peso_dist_parada decaimento_rcl tenure_rota iteracoes_tabu num_trajetorias periodo_troca .PARENT.
// 92   92       126    0.9244          0.8697       0.0286       0.1645        0.4646       0.0319      0.4017    0.2182             0.4341           0.1587           0.2602         0.3623       0.486             63               2            33       54

// Description of the best-so-far configuration:
//    .ID. limiar_estagnacao_mutacao injecao_extra vizinhos_div decaimento_rotas_rep mutacao_onibus decaimento_solucao_inicial decaimento_rota_inicial tamanho_gaveta periodo_arquivamento limiar_gaveta engavetados_k semelhanca_gaveta recompensa_novo_melhor recompensa_melhora recompensa_aceito decaimento_peso tamanho_segmento .PARENT.
// 16   16                         3         0.216           11                0.924         0.0364                     0.6487                  0.6623             25                   16            17             3            0.1293                10.0104             3.0069            1.5714          0.7766                6       NA


struct ParametrosAlgoritmo {

    int seed = 123;
    bool log = true;
    // ================================================================
    // ALGORITMO GENÉTICO
    // ================================================================

    // Parâmetros gerais
    int numeroMaxGeracoes = 87;
    int tamanhoDaPopulacao = 120;
    double probabilidadeCrossover = 0.95;
    int elitismo = 1;

    // Taxas de mutação
    double probabilidadeMutacao = 0.01;
    double pisoTaxaMutacao = 0.0286;
    double tetoTaxaMutacao = 0.1645;
    double probabilidadeMacroMutacao = 0.2;
    int limiarEstagnacaoMutacao = 3;

    double lambdaFreq = 2.0;      // 0 = desliga 
    double decaimentoFreq = 0.1;  // evaporação por geração
    double elitePctFreq = 0.1;    // fração da população que reforça
    double distMediaEntreParadas = 10;
    double LambdaRota = 0.5;
    double BetaCarga = 0.5;

    // Injeção de indivíduos novos
    double porcentagemBaseInjecao = 0.0319;
    double porcentagemExtraInjecao = 0.216;
    double porcentagemMaximaNovosIndividuos = 0.4017;

    // Gaveta de indivíduos antigos
    int tamanhoGaveta = 25;
    int periodoArquivamento = 16;
    int limiarGaveta = 17;
    int engavetadosK = 3;
    double semelhancaGaveta = 0.1293;

    // Penalidade - construção de rotas
    double penalidadePorRotaAtiva = 1.0;
    int limiarParadasPorRotas = 1;
    double penalidadeRotaExtraPequena = -10.0;
    double penalidadeDesbalanceamento = 0.0;

    // Penalidade - paradas
    int limiteParadaPorRota = 1;
    double penalidadeParadaEntreRota = 20.0;
    double penalidadeParadaMesmaRota = 20.0;
    double PenalidadeParadasPercorridasGlobal = 1.0;

    double PenalidadeEstouroCapacidade = 10.0;

    // Torneio
    double porcentagemTorneioK = 0.2182;
    double taxaDecaimentoTorneio = 0.4341;

    // Seleção
    double pesoDistParada = 0.2602;
    int nVizinhosDiv = 11;
    double pesoDiversidade = 0.1587;
    double epsClone = 1e-9;

    // Reprodução
    double probabilidadeCrossoverBloco = 0.86;
    double taxaDecaimentoRotasRep = 0.924;
    double probabilidadeMutacaoOnibus = 0.0364;

    // Solução inicial
    double taxaDecaimentoSolucaoInicial = 0.64;
    double taxaDecaimentoRotaInicial = 0.66;


    // ================================================================
    // BUSCA TABU
    // ================================================================

    double tenureTaxaTamRota = 0.486;
    int iteracoesTabu = 180;

    int NumTrajetorias = 3;
    int PeriodoTroca = 15; 
    int ForcaPerturbacaoTabu = 20;
    int LimiarEstagnacaoTabu = 10; 
};

class infoSBRP;

struct Individuo {

    // índice = aluno
    // valor = parada escolhida para esse aluno
    vector<int> atrAlunoParada;

    // índice = aluno
    // valor = rota que transporta esse aluno
    vector<int> atrAlunoRota;

    // indice = rota
    // valor = onibus
    vector<int> atrOnibusRota;
    
    vector<bool> rotaViavel;

    int alunosInviaveisQuant;

	vector<int> alunoPorRota;

    // Apenas armazenado após a avaliação pelo Tabu
    vector<vector<int>> rotasFeitas;
	
	// custo final de penalidade aplicada
    double fitness = numeric_limits<double>::max(); // fitness + penalidades
    double fitnessPuro = numeric_limits<double>::max(); // fitness puro vindo do tabu
	double penalidadeFitness; // soma das penalidades
};



struct Trajetoria {
    vector<int> rotaAtual;
    vector<bool> estaNaRota;
    vector<int> melhorRota;
    double melhorDistancia;
    int semMelhora = 0;
};


// +-+--+-++--+-++--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+-+-+



class Metaheuristica{
    private:
    
        infoSBRP& problema;
        std::random_device rd;
        std::mt19937 gen{rd()};

        bool log;

        DistanciasSobDemanda distancias;
        unordered_map<string, pair<double, vector<int>>> cacheRotas;
        const int LimiteExato = 10;
        
        // std::mt19937 gen;
		
        // geral
		int quantidadeMaxRota;

        // vector<int> quantAlunosPorParada;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        //                       ALGORITMO GENETICO                           //
        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
            // Parametros gerais genetico
            const int numeroMaxGeracoes;
            const int TamanhoDaPopulacao;
            const double ProbabilidadeCrossover;
            // const int Elitismo;
            
            // Taxas de mutação
            double ProbabilidadeMutacao; 
            const double PisoTaxaMutacao;
            const double TetoTaxaMutacao;
            const int limiarEstagnacaoMutacao;
            const double ProbabilidadeMacroMutacao; // mutação violenta (reprodução)

            // reforço de frequencia
            const double LambdaFreq, DecaimentoFreq, ElitePctFreq;
                vector<double> freqParada;
                double freqMaxParada = 0;
                int sinalFreqParada = 1; // +1 intensifica, -1 diversifica

                double distMediaEntreParadas;
                const double LambdaRota;
                const double BetaCarga;
            
            // injeção de individuos na estagnação (imigração)
            const double PorcentagemBaseInjecao;
            const double PorcentagemExtraInjecao; 
            const double PorcentagemMaximaNovosIndividuos; 
            
            // injeção de individuos na estagnação (Antigos)
            deque<Individuo> Gaveta;
            const int TamanhoGaveta; 
            const int PeriodoArquivamento; // guarda a cada N gerações
            const int LimiarGaveta; // insere X a cada N gerações estagnadas,
            const int EngavetadosK; // quantos na gaveta vão ser renseridos 
            const double SemelhancaGaveta; //menor, mair parecido
            // quantidade a ser inserida corrigida na gambiarra pela outra mutação (feature)

            
            // Penalidade Construção de Rotas Genetico
            const double PenalidadeEstouroCapacidade;

            
                // rotas
                const double PenalidadePorRotaAtiva; // penalidade por rota ativa. quanto mais, maior é a penal
                const int LimiarParadasPorRotas; //minimo de paradas pro rota
                const double PenalidadeRotaExtraPequena; // penalidade de rota muito curta
                const double PenalidadeDesbalanceamento; // penalidade de rotas com alunos desbalanceado


                // // paradas
                const int LimiteParadaPorRota; // minimo de repeticao de parada entre rotas
                const double PenalidadeParadaEntreRota; // diferente rota
                const double PenalidadeParadaMesmaRota; // mesma roota
                const double PenalidadeParadasPercorridasGlobal; // quantidade de paradas percorridas

            // Torneio
            const double PorcentagemTorneioK; // tamanho k com base na quantidade de individuos
            const double TaxaDecaimentoTorneio;

            // Selecao
            const double PesoDistParada;   // peso de paradas vs estrutura de rotas na distância
            const int    NVizinhosDiv;     // quantos vizinhos mais próximos entram na diversidade
            const double PesoDiversidade;   // 0 = só fitness; maior = mais diversidade
            const double EpsClone; // quanto menor, mais parecido
            
            // Reproducao
            const double ProbabilidadeCrossoverBloco; // crossover pro bloco de rotas, ou gene a gene
            const double TaxaDecaimentoRotasRep; // selecao dos blocos de rota com mais alunos
            const double ProbabilidadeMutacaoOnibus;

            // Geracao da solucao Inicial
            const double TaxaDecaimentoSolucaoInicial;
            const double TaxaDecaimentoRotaInicial;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        

        // ================================================================== //
        //                            BUSCA TABU                              //
        // ================================================================== //
        const double TenureTaxaTamRota; // tenure com base no Tamanho da rota;
        const int IteracoesTabu; 

        const int NumTrajetorias; // quantidade de instancias paralelas por cada tabu
        const int PeriodoTroca; // a cada iterações, um compartilhamento do tabu

        const int ForcaPerturbacaoTabu; // iteracoes de busca
        const int LimiarEstagnacaoTabu; // iterações estagnadas
        // ================================================================== //
        
        
    public:
	    Metaheuristica(infoSBRP& pobrema, const ParametrosAlgoritmo &p) : problema(pobrema),
            // gen(p.seed),
            log(p.log),
            numeroMaxGeracoes(p.numeroMaxGeracoes),
            TamanhoDaPopulacao(p.tamanhoDaPopulacao),
            ProbabilidadeCrossover(p.probabilidadeCrossover),
            // Elitismo(p.elitismo),

            ProbabilidadeMutacao(p.probabilidadeMutacao),
            PisoTaxaMutacao(p.pisoTaxaMutacao),
            TetoTaxaMutacao(p.tetoTaxaMutacao),
            limiarEstagnacaoMutacao(p.limiarEstagnacaoMutacao),
            ProbabilidadeMacroMutacao(p.probabilidadeMacroMutacao),

            LambdaFreq(p.lambdaFreq), 
            DecaimentoFreq(p.decaimentoFreq),
            ElitePctFreq(p.elitePctFreq),
            LambdaRota(p.LambdaRota),
            BetaCarga(p.BetaCarga),
            
            PorcentagemBaseInjecao(p.porcentagemBaseInjecao),
            PorcentagemExtraInjecao(p.porcentagemExtraInjecao),
            PorcentagemMaximaNovosIndividuos(p.porcentagemMaximaNovosIndividuos),
            
            TamanhoGaveta(p.tamanhoGaveta),
            PeriodoArquivamento(p.periodoArquivamento),
            LimiarGaveta(p.limiarGaveta),
            EngavetadosK(p.engavetadosK),
            SemelhancaGaveta(p.semelhancaGaveta),

            PenalidadeEstouroCapacidade(p.PenalidadeEstouroCapacidade),
            
            PenalidadePorRotaAtiva(p.penalidadePorRotaAtiva),
            LimiarParadasPorRotas(p.limiarParadasPorRotas),
            PenalidadeRotaExtraPequena(p.penalidadeRotaExtraPequena),
            PenalidadeDesbalanceamento(p.penalidadeDesbalanceamento),
            LimiteParadaPorRota(p.limiteParadaPorRota),
            PenalidadeParadaEntreRota(p.penalidadeParadaEntreRota),
            PenalidadeParadaMesmaRota(p.penalidadeParadaMesmaRota),
            PenalidadeParadasPercorridasGlobal(p.PenalidadeParadasPercorridasGlobal),
            
            PorcentagemTorneioK(p.porcentagemTorneioK),
            TaxaDecaimentoTorneio(p.taxaDecaimentoTorneio),
            
            PesoDistParada(p.pesoDistParada),
            NVizinhosDiv(p.nVizinhosDiv),
            PesoDiversidade(p.pesoDiversidade),
            EpsClone(p.epsClone),
            ProbabilidadeCrossoverBloco(p.probabilidadeCrossoverBloco),
            TaxaDecaimentoRotasRep(p.taxaDecaimentoRotasRep),
            ProbabilidadeMutacaoOnibus(p.probabilidadeMutacaoOnibus),
            TaxaDecaimentoSolucaoInicial(p.taxaDecaimentoSolucaoInicial),
            TaxaDecaimentoRotaInicial(p.taxaDecaimentoRotaInicial),

            TenureTaxaTamRota(p.tenureTaxaTamRota),
            IteracoesTabu(p.iteracoesTabu),
            NumTrajetorias(p.NumTrajetorias),
            PeriodoTroca(p.PeriodoTroca),
            ForcaPerturbacaoTabu(p.ForcaPerturbacaoTabu),
            LimiarEstagnacaoTabu(p.LimiarEstagnacaoTabu)
        {
			quantidadeMaxRota = pobrema.quantidadeRotas; // n precisava, mas depois arrumo

            freqParada.assign(pobrema.quantidadeParadas, 0.0);

            iniciaDistancias();
            
            int distTotal = 0, qq = 0;
            for(int i = 0; i < problema.quantidadeParadas; ++i){
                for(int j = i + 1; j < problema.quantidadeParadas; ++j){
                    distTotal += distancias.distancia[i][j];
                    ++qq;
                }
            }

            distMediaEntreParadas = distTotal / qq;
		}

        int qr(){return quantidadeMaxRota;};
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        pair<vector<double>, Individuo> AG(int opc);
		
        Individuo geraSolucaoInicial();
        vector<Individuo> popIni(int tamanhoPopulacao);
        double aplicaPenalidades(Individuo& individuo);
        bool maisViavel(const Individuo &a, const Individuo &b);
        int selecionaTorneio(vector<Individuo> &populacao);
        vector<Individuo> novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais);
        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);
        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao);
        void mutacaoIndividuo(Individuo &filho, bool forcarMacro = false);
        void injecaoNovosIndividuos(int opc, int geracoesSemMelhora, int i, vector<Individuo> &novaPopulacao);
        double distanciaIndividuos(const Individuo& a, const Individuo& b);
        vector<Individuo> selecionaSobreviventes(vector<Individuo>& pool);

        double pesoFreq(int parada);
        int sorteiaParada(const estudante& aluno);
        void atualizaFrequencias(const vector<Individuo>& popOrdenada);
        int sorteiaRota(const vector<int>& candidatas, const Individuo& filho, int parada);
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        void iniciaDistancias();
        double distanciaSequencia(const vector<int>& paradas);
        double rotaExata(const vector<int>& paradas, vector<int>& ordem);
        vector<int> insercaoMaisBarata(const vector<int>& paradas);
        double tabuSequencia(vector<int>& sequencia, int iteracoes);
        double melhorOrdemRota(vector<int> paradas, vector<int>& ordem);
        vector<int> trajetoReal(const vector<int>& ordem);


       double buscaTabu(Individuo& configParada);

        // -+-+--+-+-+-+-++-+
        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados, vector<double> valores);

};