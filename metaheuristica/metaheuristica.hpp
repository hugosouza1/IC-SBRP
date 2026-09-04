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
#include <climits>

#include <unordered_set>
#include <unordered_map>

#include "../SBRP.hpp"

inline std::random_device rd;
inline std::mt19937 gen(12345);


using namespace chrono;

class infoSBRP;


// struct Individuo {

//     // índice = aluno
//     // valor = parada escolhida para esse aluno
//     vector<int> atrAlunoParada;

//     // índice = aluno
//     // valor = rota que transporta esse aluno
//     vector<int> atrAlunoRota;

//     vector<double> intensidadePermutaRota; // pro tabu

//     double fitness = numeric_limits<double>::max();

//     vector<bool> rotaViavel;

//     int alunosInviaveisQuant;

//     // Apenas armazenado após a avaliação pelo Tabu
//     vector<vector<int>> rotasFeitas;
// };

// +-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+--++--
enum TipoMovimento{
    INSERIR,
    REMOVER
};

struct Movimento{

    TipoMovimento tipo;

    int posicao;
    
    int anterior;
    int parada;
    int proximo;

    int delta;
};



// 64 bits:
// 1 : inserir / remover
// 21: anterior
// 21: parada
// 21: proximo
// arco : anterior -> parada -> proximo
using ChaveTabu = uint64_t;


// +-+--+-++--+-++--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+-+-+

class Metaheuristica{
	private:
		infoSBRP& problema;
		
		int quantidadeMaxRota;



    public:
	    Metaheuristica(infoSBRP& p) : problema(p) {
			quantidadeMaxRota = (p.quantidadeAlunos + p.Q) / p.Q;
		}
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        Individuo warmStart();

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        Individuo AG();
		
        Individuo geraSolucaoInicial();

        vector<Individuo> popIni(int tamanhoPopulacao);

        bool maisViavel(const Individuo &a, const Individuo &b);

        int selecionaTorneio(vector<Individuo> &populacao, double chanceAceitarPior);

        vector<Individuo> novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais, int tamanhoPopulacao, int elitismo);

        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);

        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao, int tamanhoPopulacao, double mutacao, double crossoverProb);

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		vector<int> contrucaoRota(vector<int> paradasMinimas, bool& sucesso);

		vector<int> bfs(int a, int b);

		vector<vector<int>> caminhosIniciais(vector<vector<int>> conjuntoParadas, vector<bool>& sucesso);

        Movimento melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu, int>& tabu, int melhorDistanciaGlobal, int distanciaAtual, int iteracao);

    	Movimento melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, int melhorDistanciaGlobal, int distanciaAtual, int iteracao);

        Movimento melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu, int>& tabu, int melhorDistanciaGlobal, int iteracao);

		void aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov);

		double buscaTabu(Individuo& configParada);

        ChaveTabu chaveTabu(TipoMovimento tipo, int anterior, int parada, int proximo);

        bool movimentoTabu(unordered_map<ChaveTabu, int>& tabu, TipoMovimento tipo, int anterior, int parada, int proximo, int iteracao );
        
        void atualizaTabu( unordered_map<ChaveTabu, int>& tabu, Movimento mov, int tenure, int iteracao);

        void pertubacaoRota(vector<vector<int>> &rota, vector<double> intensidade);

        // -+-+--+-+-+-+-++-+

        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados);

};