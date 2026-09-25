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

class Metaheuristica{
	private:
		infoSBRP& problema;
		
        // geral
		int quantidadeMaxRota;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        //                       ALGORITMO GENETICO                           //
        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
            // Parametros gerais genetico
            const int numeroMaxGeracoes = 150;
            const int TamanhoDaPopulacao = 200;
            const double ProbabilidadeCrossover = 0.85;
            const int Elitismo = 1;
            
            // Taxas de mutação
            double ProbabilidadeMutacao = 0.01; 
            const double PisoTaxaMutacao = 0.01;
            const double TetoTaxaMutacao  = 0.3;
            const int limiarEstagnacaoMutacao = 3;
            const double ProbabilidadeMacroMutacao = 0.05; // mutação violenta (reprodução)
            
            // injeção de individuos na estagnação
            const double PorcentagemBaseInjecao  = 0.1;
            const double PorcentagemExtraInjecao = 0.05; 
            const double PorcentagemMaximaNovosIndividuos = 0.25; 
            
            // Penalidade Construção de Rotas Genetico
                // rotas
                const double PenalidadePorRotaAtiva = 10.0; // penalidade por rota ativa. quanto mais, maior é a penal
                const int LimiarParadasPorRotas = 1; //minimo de paradas pro rota
                const double PenalidadeRotaExtraPequena = 10.0; // penalidade de rota muito curta
                const double PenalidadeDesbalanceamento = 10.0; // penalidade de rotas com alunos desbalanceado
                // paradas
                const int limiteParadaPorRota = 1; // minimo de repeticao de parada entre rotas
                const double PenalidadeParadaEntreRota = 100.0; // diferente rota
                const double PenalidadeParadaMesmaRota = 50.0; // mesma roota

            // Torneio
            const double PorcentagemTorneioK = 0.03; // tamanho k com base na quantidade de individuos
            const double TaxaDecaimentoTorneio = 0.25;
            
            // Reproducao
            const double ProbabilidadeCrossoverBloco = 0.5; // crossover pro bloco de rotas, ou gene a gene
            const double TaxaDecaimentoRotasRep = 0.6; // selecao dos blocos de rota com mais alunos
            const double ProbabilidadeMutacaoOnibus = 0.3;

            // Geracao da solucao Inicial
            const double TaxaDecaimentoSolucaoInicial = 0.4;
            const double TaxaDecaimentoRotaInicial    = 0.4;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
        

        // ================================================================== //
        //                            BUSCA TABU                              //
        // ================================================================== //
        const int TamanhoRCL = 200;
        const double TaxaDecaimentoRCL = 0.25; // quanto maior, mais pende pros melhores. menor, mais uniforme
        const double TenureTaxaTamRota = 0.15; // tenure com base no Tamanho da rota;
        const int IteracoesTabu = 70; 
        // ================================================================== //
        


        vector<int> quantAlunosPorParada;
        
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

        // vector<Movimento> candidatosInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
        // vector<Movimento> candidatosRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
        // vector<Movimento> candidatosTroca(vector<int>& rota, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        // Movimento melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
        // Movimento melhorTroca(vector<int>& rota, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
    	// Movimento melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

        Movimento melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, double melhorDistanciaGlobal, int iteracao);

		void aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov);

		double buscaTabu(Individuo& configParada);

        ChaveTabu chaveTabu(TipoMovimento tipo, int anterior, int parada, int proximo);

        bool movimentoTabu(unordered_map<ChaveTabu, int>& tabu, TipoMovimento tipo, int anterior, int parada, int proximo, int iteracao );
        
        void atualizaTabu( unordered_map<ChaveTabu, int>& tabu, Movimento mov, int tenure, int iteracao);

        void pertubacaoRota(vector<vector<int>> &rota, vector<double> intensidade);

        // -+-+--+-+-+-+-++-+
        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados, vector<double> valores);

};