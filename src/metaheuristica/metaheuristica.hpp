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

#include "../SBRP.hpp"

inline std::random_device rd;
inline std::mt19937 gen(rd());


using namespace chrono;
using namespace std;

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

    vector<double> intensidadePermutaRota; // pro tabu

    double fitness = numeric_limits<double>::max();

    vector<bool> rotaViavel;

    int alunosInviaveisQuant;
	vector<int> alunoPorRota;

    // Apenas armazenado após a avaliação pelo Tabu
    vector<vector<int>> rotasFeitas;
	
	// custo final de penalidade aplicada
	double penalidadeFitness;
};


// +-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+--++--
enum TipoMovimento{
    INSERIR,
    REMOVER,
    TROCAR,
    SUBSTITUIR,
    MOVER
};

struct Movimento{

    TipoMovimento tipo;

    int posicao;     // REMOVER / INSERIR: posição do alvo
    int posOrigem;   // MOVER: posição de onde a parada sai
    int posDestino;  // MOVER: posição (gap) pra onde vai

    int anterior, parada, proximo; // chave tabu / valores da operação

    int paradaAntiga; // SUBSTITUIR: parada removida
    int paradaNova;   // SUBSTITUIR / INSERIR / MOVER: parada envolvida

    int trocaA, trocaB; // TROCAR: posições trocadas

    double delta;
};


// 64 bits:
// 4 : inserir / remover
// 20: anterior
// 20: parada
// 20: proximo
// arco : anterior -> parada -> proximo
using ChaveTabu = uint64_t;

// +-+--+-++--+-++--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+-+-+

struct ALNS{
    unordered_map<TipoMovimento, double> pesoOperador = {{INSERIR, 1.0}, {REMOVER, 1.0}, {TROCAR, 1.0}, {SUBSTITUIR, 1.0}, {MOVER, 1.0}};
    unordered_map<TipoMovimento, double> scoreAcumulado = {{INSERIR, 0.0}, {REMOVER, 0.0}, {TROCAR, 0.0}, {SUBSTITUIR, 0.0}, {MOVER, 0.0}};
    unordered_map<TipoMovimento, int> usosNoSegmento = {{INSERIR, 0}, {REMOVER, 0}, {TROCAR, 0}, {SUBSTITUIR, 0}, {MOVER, 0}};
};


// +-+--+-++--+-++--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+-+-+



class Metaheuristica{
	private:
		infoSBRP& problema;
		
        // geral
		int quantidadeMaxRota;

        vector<int> quantAlunosPorParada;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        //                       ALGORITMO GENETICO                           //
        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
            // Parametros gerais genetico
            const int numeroMaxGeracoes = 100;
            const int TamanhoDaPopulacao = 200;
            const double ProbabilidadeCrossover = 0.90;
            const int Elitismo = 3;
            
            // Taxas de mutação
            double ProbabilidadeMutacao = 0.1; 
            const double PisoTaxaMutacao = 0.08;
            const double TetoTaxaMutacao  = 0.64;
            const int limiarEstagnacaoMutacao = 5;
            const double ProbabilidadeMacroMutacao = 0.25; // mutação violenta (reprodução)
            
            // injeção de individuos na estagnação (imigração)
            const double PorcentagemBaseInjecao  = 0.05;
            const double PorcentagemExtraInjecao = 0.05; 
            const double PorcentagemMaximaNovosIndividuos = 0.40; 
            
            // injeção de individuos na estagnação (Antigos)
            deque<Individuo> gaveta;
            const int tamanhoGaveta = 50; 
            const int PeriodoArquivamento = 5; // guarda a cada N gerações
            const int limiarGaveta = 5; // insere X a cada N gerações estagnadas,
            const int EngavetadosK = 2; // quantos na gaveta vão ser renseridos 
            const double SemelhancaGaveta = 0.05;
            // quantidade a ser inserida corrigida na gambiarra pela outra mutação (feature)

            
            // Penalidade Construção de Rotas Genetico
                // rotas
                const double PenalidadePorRotaAtiva = 1.0; // penalidade por rota ativa. quanto mais, maior é a penal
                const int LimiarParadasPorRotas = 1; //minimo de paradas pro rota
                const double PenalidadeRotaExtraPequena = -10.0; // penalidade de rota muito curta
                const double PenalidadeDesbalanceamento = 0.0; // penalidade de rotas com alunos desbalanceado
                // paradas
                const int limiteParadaPorRota = 1; // minimo de repeticao de parada entre rotas
                const double PenalidadeParadaEntreRota = 500.0; // diferente rota
                const double PenalidadeParadaMesmaRota = 50.0; // mesma roota

            // Torneio
            const double PorcentagemTorneioK = 0.10; // tamanho k com base na quantidade de individuos
            const double TaxaDecaimentoTorneio = 0.40;

            // Selecao
            const double PesoDistParada  = 0.5;   // peso de paradas vs estrutura de rotas na distância
            const int    NVizinhosDiv    = 3;     // quantos vizinhos mais próximos entram na diversidade
            const double PesoDiversidade = 0.8;   // 0 = só fitness; maior = mais diversidade
            const double EpsClone        = 1e-9;
            
            // Reproducao
            const double ProbabilidadeCrossoverBloco = 0.65; // crossover pro bloco de rotas, ou gene a gene
            const double TaxaDecaimentoRotasRep = 0.40; // selecao dos blocos de rota com mais alunos
            const double ProbabilidadeMutacaoOnibus = 0.40;

            // Geracao da solucao Inicial
            const double TaxaDecaimentoSolucaoInicial = 0.2;
            const double TaxaDecaimentoRotaInicial    = 0.2;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        

        // ================================================================== //
        //                            BUSCA TABU                              //
        // ================================================================== //
        const int TamanhoRCL = 150;
        const double TaxaDecaimentoRCL = 0.25; // quanto maior, mais pende pros melhores. menor, mais uniforme
        const double TenureTaxaTamRota = 0.05; // tenure com base no Tamanho da rota;
        const int IteracoesTabu = 300; 

        const double RecompensaNovoMelhor = 3.0; // achou novo melhor global da rota
        const double RecompensaMelhora    = 1.5; // melhorou o atual, mesmo sem ser o melhor
        const double RecompensaAceito     = 0.5; // aceito, mas não melhorou

        const double FatorDecaimentoPeso = 0.70; // memória: quanto do peso antigo mantém
        const int TamanhoSegmento        = 10; // quantas iterações tem decaimento
        // ================================================================== //
        
        
    public:
	    Metaheuristica(infoSBRP& p) : problema(p) {
			quantidadeMaxRota = p.quantidadeRotas; // n precisava, mas depois arrumo
            

            quantAlunosPorParada.assign(p.quantidadeAlunos, 0);
            for(auto aluno : p.alunosParadas){
                for(int k = 0; k < aluno.paradasPossiveis.size(); ++k){
                    quantAlunosPorParada[aluno.paradasPossiveis[k].first]++;
                }
            }
		}

        int qr(){return quantidadeMaxRota;};
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        pair<vector<double>, Individuo> AG(int opc);
		
        Individuo geraSolucaoInicial();

        double distancia(vector<int>& caminho, vector<vector<double>>& grafo);

        vector<Individuo> popIni(int tamanhoPopulacao);
            
        double aplicaPenalidades(Individuo& individuo);

        bool maisViavel(const Individuo &a, const Individuo &b);

        int selecionaTorneio(vector<Individuo> &populacao);

        vector<Individuo> novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais);

        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);

        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao);

        void mutacaoIndividuo(Individuo &filho, vector<double> intensidadePermutacao1, vector<double> intensidadePermutacao2,  bool forcarMacro = false);

        void injecaoNovosIndividuos(int opc, int geracoesSemMelhora, int i, vector<double> &fitnessPop, vector<Individuo> &novaPopulacao);

        double distanciaIndividuos(const Individuo& a, const Individuo& b);

        vector<Individuo> selecionaSobreviventes(vector<Individuo>& pool);

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		vector<int> contrucaoRota(vector<int> paradasMinimas, bool& sucesso);

		vector<int> bfs(int a, int b);

        // vector<int> dijkstra(int a, int b);

		vector<vector<int>> caminhosIniciais(Individuo &configParada, vector<bool>& sucesso, vector<vector<int>> *paradaDasRotas = {});

        void finalizaSolucao(Individuo& configParada, vector<vector<int>>& rotas, const vector<bool>& sucesso);


        vector<Movimento> candidatosRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        vector<Movimento> candidatosInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);


        vector<Movimento> candidatosTroca(vector<int>& rota, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        vector<Movimento> candidatosMover(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        vector<Movimento> candidatosSubstituir(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        Movimento melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, int iteracao, ALNS &Adapt);

		void aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov);

		double buscaTabu(Individuo& configParada);

        ChaveTabu chaveTabu(TipoMovimento tipo, int anterior, int parada, int proximo);

        bool movimentoTabu(unordered_map<ChaveTabu, int>& tabu, TipoMovimento tipo, int anterior, int parada, int proximo, int iteracao );
        
        void atualizaTabu( unordered_map<ChaveTabu, int>& tabu, Movimento mov, int tenure, int iteracao);

        void pertubacaoRota(vector<vector<int>> &rota, vector<double> intensidade);

        // -+-+--+-+-+-+-++-+
        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados, vector<double> valores);

};