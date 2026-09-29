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
#include <sstream>
#include <unordered_map>


using namespace std;

struct estudante{
    int id; // estudante A

    // parada, distancia
    vector<pair<int, double>> paradasPossiveis;
};


class Metaheuristica;
class ModeloMatematico;

class infoSBRP{
	friend class Metaheuristica;
	friend class ModeloMatematico;

	private:
	    // matriz de distâncias/custos entre paradas
		int quantidadeArestas;
	    vector<vector<double>> grafoParadas;

	    // estudantes
	    vector<estudante> alunosParadas;

	    int quantidadeParadas;
	    int quantidadeAlunos;
	    int quantidadeRotas;

	    int quantidadeOnibus;
		vector<int> capacidadeOnibus;

        // precisa de um teto pro step do solver
	    int quantidadePassos; 

	    // capacidade do ônibus
	    int Q;

	public:

	    void leitura(string arquivoEntrada);
};
