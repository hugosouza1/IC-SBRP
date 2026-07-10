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

#include "../SBRP.hpp"


class infoSBRP;


struct Individuo {

    // índice = aluno
    // valor = parada escolhida para esse aluno
    vector<int> atrAlunoParada;

    // índice = aluno
    // valor = rota que transporta esse aluno
    vector<int> atrAlunoRota;

    int fitness;

    // Apenas armazenado após a avaliação pelo Tabu
    vector<vector<int>> rotasFeitas;
};

// +-+--+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+--++--
enum TipoMovimento{
    INSERIR,
    REMOVER,
    RELOCATE
};

// A -> B -> C // busca Tabu
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




class Metaheuristica{
	private:
		infoSBRP& problema;
		
		int quantidadeMaxRota;



    public:
	    Metaheuristica(infoSBRP& p) : problema(p) {
			quantidadeMaxRota = p.quantidadeAlunos / p.Q * 1.5; 
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        void AG();
		
        Individuo geraSolucaoInicial();

        vector<Individuo> popIni(int tamanhoPopulacao);
		
		
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		vector<int> contrucaoRota(set<int> paradasMinimas);

		vector<int> bfs(int a, int b);

		vector<vector<int>> caminhosIniciais(vector<set<int>> conjuntoParadas);

		Movimento melhorInsercao(vector<int>& rota, vector<bool>& estaNaRota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		Movimento melhorRemocao(vector<int>& rota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		Movimento melhorRelocate(vector<int>& rota, vector<int>& tabuParada, int melhorDistanciaGlobal, int distanciaAtual);

		Movimento melhorVizinho(vector<int>& rota, vector<bool>& estaNaRota, vector<bool>& paradaObrigatoria, vector<int>& tabuParada, int melhorDistanciaGlobal);

		void aplicaMovimento(vector<int>& rota, vector<bool>& estaNaRota, Movimento mov);

		void atualizaTabu(vector<int>& tabuParada, int paradaMovida, int tenure);

		int buscaTabu(Individuo& configParada);

};