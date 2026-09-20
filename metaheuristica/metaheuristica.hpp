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

class infoSBRP;


// +-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+--++--
enum TipoMovimento{
    INSERIR,
    REMOVER,
    TROCAR
};

struct Movimento{

    TipoMovimento tipo;

    int posicao;
    
    int anterior;
    int parada;
    int proximo;

    int trocaA;
    int trocaB;

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
		
		int quantidadeMaxRota;



    public:
	    Metaheuristica(infoSBRP& p) : problema(p) {
			quantidadeMaxRota = p.quantidadeRotas; // n precisava, mas depois arrumo
		}
        int qr(){return quantidadeMaxRota;};
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        Individuo AG(int opc);
		
        Individuo geraSolucaoInicial();

        double distancia(vector<int>& caminho, vector<vector<double>>& grafo);

        vector<Individuo> popIni(int tamanhoPopulacao);
            
        double aplicaPenalidades(Individuo& individuo);

        bool maisViavel(const Individuo &a, const Individuo &b);

        int selecionaTorneio(vector<Individuo> &populacao, double chanceAceitarPior);

        vector<Individuo> novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais, int tamanhoPopulacao, int elitismo);

        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);

        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao, int tamanhoPopulacao, double mutacao, double crossoverProb);

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		vector<int> contrucaoRota(vector<int> paradasMinimas, bool& sucesso);

		vector<int> bfs(int a, int b);

		vector<vector<int>> caminhosIniciais(Individuo &configParada, vector<bool>& sucesso, vector<vector<int>> *paradaDasRotas = {});

        void finalizaSolucao(Individuo& configParada, vector<vector<int>>& rotas, const vector<bool>& sucesso);

        vector<Movimento> candidatosInsercao(vector<int>& rota, vector<bool>& estaNaRota, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
        vector<Movimento> candidatosRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);
        vector<Movimento> candidatosTroca(vector<int>& rota, unordered_map<ChaveTabu,int>& tabu, double melhorDistanciaGlobal, double distanciaAtual, int iteracao);

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
        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados);

};