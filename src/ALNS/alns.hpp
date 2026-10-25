#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <random>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <functional>
#include <cmath>
#include <set>

#include "../SBRP.hpp"
#include "../distancias.hpp"

using namespace std;
using namespace chrono;

struct ParametrosALNS {

    int seed = 123;
    bool log = true;

    // Orçamento
    double tempoMaxSegundos = 20.0;
    long maxIteracoes = 100000;

    // Destruição
    double fracaoMinRemocao = 0.05; // fração dos alunos removidos por iteração
    double fracaoMaxRemocao = 0.30;

    // Aprendizado dos pesos dos operadores
    int tamanhoSegmento = 100;       // iterações entre cada atualização dos pesos
    double taxaAprendizado = 0.1;    // w = (1 - taxa) * w + taxa * pontos / usos
    double recompensaNovoMelhor = 33;
    double recompensaMelhora = 9;    // melhor que a atual
    double recompensaAceito = 3;     // pior, mas aceita pelo SA

    // Aceitação (simulated annealing)
    double fracaoTemperaturaInicial = 0.05; // aceita solução 5% pior com prob 0.5 no início
    double taxaResfriamento = 0.9995;

    double penalidadePorRotaAtiva = 1.0;
    double penalidadeEstouroCapacidade = 1000.0;
};


struct SolucaoALNS {

    // índice = aluno (-1 = ainda não alocado)
    vector<int> atrAlunoParada;
    vector<int> atrAlunoRota;

    // índice = rota
    vector<int> atrOnibusRota;
    vector<vector<int>> paradasDaRota;   // paradas em ordem de visita (sem o depósito)
    vector<int> alunoPorRota;
    vector<double> distanciaRota;

    // índice = [rota][parada]
    vector<vector<int>> alunosNaParada;

    double fitness = numeric_limits<double>::max(); // distância + penalidades
    double fitnessPuro = numeric_limits<double>::max(); // só distância
    double penalidadeFitness = 0.0;
};


struct CandidatoInsercao {
    double custo = INFINITO;
    int parada = -1;
    int rota = -1;
    int posicao = 0; // posição na sequência da rota
};


struct ConfigReparo {
    string nome;
    int regretK;                    // 1 = gulosa, 2 ou 3 = regret-k
    double amplitudeRuido;          // 0 = sem ruído; 0.2 => custo * U(0.8, 1.2)
    double fatorPenalidadeParadaNova; // x distância média, pra consolidar paradas
};

class infoSBRP;

class ALNS{
    private:

        infoSBRP& problema;
        mt19937 gen;

        bool log;
        int quantidadeMaxRota;

        DistanciasSobDemanda distancias;
        double distanciaMaxima = 1;
        double distanciaMedia = 1;

        // Orçamento
        const double TempoMaxSegundos;
        const long MaxIteracoes;

        // Destruição
        const double FracaoMinRemocao;
        const double FracaoMaxRemocao;

        // Aprendizado
        const int TamanhoSegmento;
        const double TaxaAprendizado;
        const double RecompensaNovoMelhor;
        const double RecompensaMelhora;
        const double RecompensaAceito;

        // Aceitação
        const double FracaoTemperaturaInicial;
        const double TaxaResfriamento;

        // Penalidades
        const double PenalidadePorRotaAtiva;
        const double PenalidadeEstouroCapacidade;

        // Operadores
        using Destruicao = function<vector<int>(SolucaoALNS&, int)>;

        vector<pair<string, Destruicao>> destruicoes;
        vector<ConfigReparo> reparos;

    public:
        ALNS(infoSBRP& pobrema, const ParametrosALNS &p) : problema(pobrema),
            gen(p.seed),
            log(p.log),
            TempoMaxSegundos(p.tempoMaxSegundos),
            MaxIteracoes(p.maxIteracoes),
            FracaoMinRemocao(p.fracaoMinRemocao),
            FracaoMaxRemocao(p.fracaoMaxRemocao),
            TamanhoSegmento(p.tamanhoSegmento),
            TaxaAprendizado(p.taxaAprendizado),
            RecompensaNovoMelhor(p.recompensaNovoMelhor),
            RecompensaMelhora(p.recompensaMelhora),
            RecompensaAceito(p.recompensaAceito),
            FracaoTemperaturaInicial(p.fracaoTemperaturaInicial),
            TaxaResfriamento(p.taxaResfriamento),
            PenalidadePorRotaAtiva(p.penalidadePorRotaAtiva),
            PenalidadeEstouroCapacidade(p.penalidadeEstouroCapacidade)
        {
            quantidadeMaxRota = pobrema.quantidadeRotas;
            distancias.inicia(pobrema.grafoParadas);
            registraOperadores();
        }

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        SolucaoALNS executar();

        void imprimeSolucao(SolucaoALNS& sol);

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        vector<int> caminhoReal(int origem, int destino) const;

        double sorteio01();

        int sorteioInteiro(int a, int b);

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        int capacidadeRota(const SolucaoALNS& s, int rota) const;

        double distanciaSequencia(const vector<int>& paradas) const;

        void avaliar(SolucaoALNS& s) const;

        SolucaoALNS solucaoVazia();

        pair<double, int> melhorPosicaoInsercao(const vector<int>& paradas, int parada) const;

        CandidatoInsercao avaliaInsercao(const SolucaoALNS& s, int parada, int rota, double penalidadeParadaNova) const;

        void inserirAluno(SolucaoALNS& s, int aluno, const CandidatoInsercao& c);

        void removerAluno(SolucaoALNS& s, int aluno);

        vector<int> alunosAtribuidos(const SolucaoALNS& s) const;

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        vector<int> destruicaoAleatoria(SolucaoALNS& s, int quantidade);

        vector<int> destruicaoPior(SolucaoALNS& s, int quantidade);

        vector<int> destruicaoParada(SolucaoALNS& s, int quantidade);

        vector<int> destruicaoRota(SolucaoALNS& s, int quantidade);

        vector<int> destruicaoRelacionada(SolucaoALNS& s, int quantidade); // Shaw

        void reparar(SolucaoALNS& s, vector<int> pendentes, const ConfigReparo& config);

        // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        void buscaLocal2Opt(SolucaoALNS& s, int rota);

        void trocaOnibusEntreRotas(SolucaoALNS& s);

        void registraOperadores();

        SolucaoALNS construirSolucaoInicial();
};