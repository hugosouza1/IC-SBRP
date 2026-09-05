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

using namespace std;

struct Individuo {

    // índice = aluno
    // valor = parada escolhida para esse aluno
    vector<int> atrAlunoParada;

    // índice = aluno
    // valor = rota que transporta esse aluno
    vector<int> atrAlunoRota;

    vector<double> intensidadePermutaRota; // pro tabu

    double fitness = numeric_limits<double>::max();

    vector<bool> rotaViavel;

    int alunosInviaveisQuant;
	vector<int> alunoPorRota;

    // Apenas armazenado após a avaliação pelo Tabu
    vector<vector<int>> rotasFeitas;
};

struct estudante{
    int id; // estudante A

    // parada, distancia
    vector<pair<int, int>> paradasPossiveis;
};


class Metaheuristica;
class ModeloMatematico;

class infoSBRP{
	friend class Metaheuristica;
	friend class ModeloMatematico;

	private:
	    // matriz de distâncias/custos entre paradas
		int quantidadeArestas;
	    vector<vector<int>> grafoParadas;

	    // estudantes
	    vector<estudante> alunosParadas;

	    int quantidadeParadas;
	    int quantidadeAlunos;
	    int quantidadeOnibus;
	    int quantidadeRotas;

        // precisa de um teto pro step
	    int quantidadePassos; 

	    // capacidade do ônibus
	    int Q;

	    // maior distância permitida (W)
	    int maxDistancia;

	public:

	    void leitura(std::string arquivoEntrada);
};
