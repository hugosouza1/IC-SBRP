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

#include "../SBRP.hpp"


class infoSBRP;


struct Individuo {

    // índice = aluno
    // valor = parada escolhida para esse aluno
    vector<int> atrAlunoParada;

    // índice = aluno
    // valor = rota que transporta esse aluno
    vector<int> atrAlunoRota;

    double fitness = numeric_limits<double>::max();

    // Apenas armazenado após a avaliação pelo Tabu
    vector<vector<int>> rotasFeitas;
};

// +-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+--++--
enum TipoMovimento{
    INSERIR,
    REMOVER
};

// A -> B -> C // busca Tabu // 
struct arcoAdj{
    int a;
    int b;
    int c;
};

struct Movimento{

    TipoMovimento tipo;

    int posicao;

    int parada;

    int novaPosicao;     // usado apenas no relocate

    int delta;

    arcoAdj arcoRemovido;
    arcoAdj arcoInserido;
};

// +-+--+-++--+-++--+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+-+-+

class Metaheuristica{
	private:
		infoSBRP& problema;
		
		int quantidadeMaxRota;



    public:
	    Metaheuristica(infoSBRP& p) : problema(p) {
			quantidadeMaxRota = p.quantidadeAlunos / p.Q * 1.5; 
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        Individuo AG();
		
        Individuo geraSolucaoInicial();

        vector<Individuo> popIni(int tamanhoPopulacao);

        int selecionaTorneio(vector<Individuo> &populacao);

        vector<Individuo> novaPopTorneioElitista(vector<Individuo> &filhos, vector<Individuo> &pais, int tamanhoPopulacao, int elitismo);

        vector<pair<int,int>> escolhendoPais(vector<Individuo> &populacao);

        vector<Individuo> reproducao(vector<pair<int,int>> &paisEscolhidos, vector<Individuo> &populacao, int tamanhoPopulacao, double mutacao, double crossoverProb);

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		vector<int> contrucaoRota(set<int> paradasMinimas, bool& sucesso);

		vector<int> bfs(int a, int b);

		vector<vector<int>> caminhosIniciais(vector<set<int>> conjuntoParadas, vector<bool>& sucesso);

		Movimento melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		Movimento melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		// Movimento melhorRelocate(vector<int>& rota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		Movimento melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal);

		void aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov);

		void atualizaTabu(vector<int>& tabuParada, int paradaMovida, int tenure);

		double buscaTabu(Individuo& configParada);



        // -+-+--+-+-+-+-++-+

        
        void imprimeSolucao(Individuo& sol, infoSBRP& dados);

};