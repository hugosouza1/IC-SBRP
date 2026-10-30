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
#include <iomanip>
#include <climits>

#include <unordered_set>
#include <unordered_map>

#include <cstdlib>

#include "../SBRP.hpp"

#include "../distancias.hpp"

using namespace chrono;
using namespace std;

inline std::random_device rd;

struct ParametrosAlgoritmo {
    unsigned int seed = rd();

    bool log = false;
    // ================================================================
    // ALGORITMO GENÉTICO
    // ================================================================

    // Parâmetros gerais
    int numeroMaxGeracoes = 10000;
    int tamanhoDaPopulacao = 25;
    double probabilidadeCrossover = 0.9398;
    int elitismo = 1;

    int janelaProtecao = 10;

    // Taxas de mutação
    double probabilidadeMutacao = 0.0481;
    double probabilidadeMacroMutacao = 0.2137;

    double lambdaFreq = 2.0;      // 0 = desliga 
    double decaimentoFreq = 0.1;  // evaporação por geração
    double elitePctFreq = 0.1;    // fração da população que reforça
    int LimiarFreqParadas = 40;  // estagnação
    double distMediaEntreParadas = 10;
    double LambdaRota = 0.5;
    double BetaCarga = 0.5;

    // Injeção de indivíduos novos
    double porcentagemInjecaoImigrantes = 0.5;
    int limiarImigrantes = 1000;

    // Gaveta de indivíduos antigos
    int tamanhoGaveta = 25;
    int periodoArquivamento = 16;
    int limiarGaveta = 17;
    int engavetadosK = 3;
    double porcentagemInjecaoGaveta = 0.2;
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

    double PenalidadeEstouroCapacidade = 85.5;

    // Torneio
    double porcentagemTorneioK = 0.256;
    double taxaDecaimentoTorneio = 0.1;

    // Seleção
    double pesoDistParada = 0.2602;
    int nVizinhosDiv = 11;
    double pesoDiversidade = 0.1587;
    double epsClone = 1e-9;

    // Reprodução
    double taxaDecaimentoRotasRep = 0.924;
    double probabilidadeMutacaoOnibus = 0.0364;

    // Solução inicial
    double taxaDecaimentoSolucaoInicial = 0.64;
    double taxaDecaimentoRotaInicial = 0.66;


    // ================================================================
    // BUSCA TABU
    // ================================================================

    double tenureTaxaTamRota = 0.1683;
    int iteracoesTabu = 84;

    int NumTrajetorias = 6;
    int PeriodoTroca = 15; 
    int ForcaPerturbacaoTabu = 20;
    int LimiarEstagnacaoTabu = 10; 
};

class infoSBRP;

struct Individuo {
    int protegidoAte = -1;   // geração até a qual não pode ser eliminado

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
        std::mt19937 gen;

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

            const int JanelaProtecao;
            
            // Taxas de mutação
            const double ProbabilidadeMutacao; 
            const double ProbabilidadeMacroMutacao; // mutação violenta (reprodução)

            // reforço de frequencia
            const double LambdaFreq, DecaimentoFreq, ElitePctFreq;
                vector<double> freqParada;
                double freqMaxParada = 0;
                const int LimiarFreqParadas;
                int sinalFreqParada = 1; // +1 intensifica, -1 diversifica

                double distMediaEntreParadas;
                const double LambdaRota;
                const double BetaCarga;
            
            // injeção de individuos na estagnação (imigração)
            double PorcentagemInjecaoImigrantes;
            const int LimiarImigrantes;
            
            // injeção de individuos na estagnação (Antigos)
            deque<Individuo> Gaveta;
            const int TamanhoGaveta; 
            const int PeriodoArquivamento; // guarda a cada N gerações
            const int LimiarGaveta; // insere X a cada N gerações estagnadas,
            const int EngavetadosK; // quantos na gaveta vão ser engavetados
            const double PorcentagemInjecaoGaveta;  // quantos % vai ser trocado
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
            const double PesoDistParada;   // peso de paradas vs estrutura de rotas na distância. 0 -> parda, 1 -> rota
            const int    NVizinhosDiv;     // quantos vizinhos mais próximos entram na diversidade
            const double PesoDiversidade;   // 0 = só fitness; maior = mais diversidade
            const double EpsClone; // quanto menor, mais parecido
            
            // Reproducao
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
            gen(p.seed),
            log(p.log),
            numeroMaxGeracoes(p.numeroMaxGeracoes),
            TamanhoDaPopulacao(p.tamanhoDaPopulacao),
            ProbabilidadeCrossover(p.probabilidadeCrossover),
            // Elitismo(p.elitismo),
            JanelaProtecao(p.janelaProtecao),

            ProbabilidadeMutacao(p.probabilidadeMutacao),
            ProbabilidadeMacroMutacao(p.probabilidadeMacroMutacao),

            LambdaFreq(p.lambdaFreq), 
            DecaimentoFreq(p.decaimentoFreq),
            ElitePctFreq(p.elitePctFreq),
            LimiarFreqParadas(p.LimiarFreqParadas),
            LambdaRota(p.LambdaRota),
            BetaCarga(p.BetaCarga),
            
            PorcentagemInjecaoImigrantes(p.porcentagemInjecaoImigrantes),
            LimiarImigrantes(p.limiarImigrantes),
            
            TamanhoGaveta(p.tamanhoGaveta),
            PeriodoArquivamento(p.periodoArquivamento),
            LimiarGaveta(p.limiarGaveta),
            EngavetadosK(p.engavetadosK),
            PorcentagemInjecaoGaveta(p.porcentagemInjecaoGaveta),
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

            double distTotal = 0; long long qq = 0;
            for(int i = 0; i < problema.quantidadeParadas; ++i)
                for(int j = i + 1; j < problema.quantidadeParadas; ++j){
                    double d = distancias[i][j];
                    if(d < INFINITO / 2){ distTotal += d; ++qq; }
                }
            distMediaEntreParadas = qq ? distTotal / qq : 1.0;
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
        vector<Individuo> novaPopTorneioElitista(vector<Individuo>& filhos, vector<Individuo>& pais, int geracao);
        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);
        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao);
        void mutacaoIndividuo(Individuo &filho, bool forcarMacro = false);
        void engavetamento(int i, vector<Individuo>& novaPopulacao);
        void injecaoNovosIndividuos(int semMelhora, int i, vector<Individuo>& novaPopulacao, int& ultimaGaveta, int& ultimaImig);
        
        double distanciaIndividuos(const Individuo& a, const Individuo& b);
        vector<Individuo> selecionaSobreviventes(vector<Individuo>& pool, int alvo);

        void crossoverRotas(const Individuo& A, const Individuo& B, Individuo& filho);

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