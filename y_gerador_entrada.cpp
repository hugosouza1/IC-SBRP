// Uso:
//   ./gerador_sbrp --n_paradas 20 --n_alunos 80 --n_onibus 4 --capacidade 40
//       --dispersao cluster --raio_max_caminhada 15 --seed 42 --out instancia_01.txt

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.141592653589 // 12 casa
#endif

using namespace std;

struct Ponto {
    double x, y;
};

struct Atribuicao {
    int alunoId;
    int paradaId;
    double distancia;
    bool dentroRaio;
};

struct Aresta {
    int a, b;
    double peso;
};

struct Parametros {
    int nParadas = 40;
    int nAlunos = 80;
    int nOnibus = 4;
    int capacidade = 40;
    double largura = 100.0;
    double altura = 100.0;
    
    string dispersao = "cluster";      // paradas: "uniforme" ou "cluster"
    // string dispersao = "uniforme";      // paradas: "uniforme" ou "cluster"
    
    string modoAlunos = "ancorado";    // "ancorado" (ao redor das paradas) ou "independente" (nuvem separada)
    // string modoAlunos = "independente";    // "ancorado" (ao redor das paradas) ou "independente" (nuvem separada)

    double raioMaxCaminhada = 20.0;
    int kVizinhos = 4; // quantos vizinhos mais proximos cada no da malha se conecta
    bool escolaCentralizada = true;
    unsigned seed = 42;
    string out = "instancia.txt";
};

static double distancia(const Ponto& a, const Ponto& b) {
    return hypot(a.x - b.x, a.y - b.y);
}

static vector<Ponto> gerarPontosUniformes(int n, double largura, double altura, mt19937& rng) {
    uniform_real_distribution<double> distX(0.0, largura);
    uniform_real_distribution<double> distY(0.0, altura);
    vector<Ponto> pontos;
    pontos.reserve(n);
    for (int i = 0; i < n; ++i) pontos.push_back({distX(rng), distY(rng)});
    return pontos;
}

static vector<Ponto> gerarPontosCluster(int n, double largura, double altura, mt19937& rng) {
    int nClusters = max(1, n / 5);
    double dispersaoCluster = min(largura, altura) * 0.08;

    uniform_real_distribution<double> distX(0.0, largura);
    uniform_real_distribution<double> distY(0.0, altura);
    vector<Ponto> centros;
    centros.reserve(nClusters);
    for (int i = 0; i < nClusters; ++i) centros.push_back({distX(rng), distY(rng)});

    uniform_int_distribution<int> escolhaCentro(0, nClusters - 1);
    vector<Ponto> pontos;
    pontos.reserve(n);
    for (int i = 0; i < n; ++i) {
        const Ponto& c = centros[escolhaCentro(rng)];
        normal_distribution<double> gaussX(c.x, dispersaoCluster);
        normal_distribution<double> gaussY(c.y, dispersaoCluster);
        double x = min(max(gaussX(rng), 0.0), largura);
        double y = min(max(gaussY(rng), 0.0), altura);
        pontos.push_back({x, y});
    }
    return pontos;
}

static vector<Ponto> gerarAlunosAoRedorDeParadas(int nAlunos,
                                                       const vector<Ponto>& paradas,
                                                       double raioMaxCaminhada,
                                                       double largura, double altura,
                                                       mt19937& rng) 
    {
    uniform_int_distribution<int> escolhaParada(0, static_cast<int>(paradas.size()) - 1);
    uniform_real_distribution<double> u01(0.0, 1.0);

    vector<Ponto> alunos;
    alunos.reserve(nAlunos);
    for (int i = 0; i < nAlunos; ++i) {
        const Ponto& casa = paradas[escolhaParada(rng)];
        double angulo = 2.0 * M_PI * u01(rng);
        // sqrt(u) para distribuicao uniforme na AREA do disco, nao concentrada no centro
        double raio = raioMaxCaminhada * sqrt(u01(rng));
        double x = min(max(casa.x + raio * cos(angulo), 0.0), largura);
        double y = min(max(casa.y + raio * sin(angulo), 0.0), altura);
        alunos.push_back({x, y});
    }
    return alunos;
}


static void atribuirAlunosAParadas(const vector<Ponto>& alunos,
                                    const vector<Ponto>& paradas,
                                    double raioMax,
                                    vector<Atribuicao>& atribuicoes,
                                    vector<int>& forcados) {
    for (size_t i = 0; i < alunos.size(); ++i) {
        vector<pair<int, double>> possiveisParadas;
        int maisProxima = -1;
        double distMaisProxima = numeric_limits<double>::infinity();

        for (size_t j = 0; j < paradas.size(); ++j) {
            double d = distancia(alunos[i], paradas[j]);
            if (d < distMaisProxima) {
                distMaisProxima = d;
                maisProxima = static_cast<int>(j);
            }
            if (d <= raioMax) {
                possiveisParadas.push_back(make_pair(static_cast<int>(j), d));
            }
        }

        bool forcado = false;
        if (possiveisParadas.empty()) {
            if (maisProxima < 0) continue; // sem paradas geradas na instancia — nada a fazer
            possiveisParadas.push_back(make_pair(maisProxima, distMaisProxima));
            forcados.push_back(static_cast<int>(i));
            forcado = true;
        }

        for (const auto& p : possiveisParadas)
            atribuicoes.push_back({static_cast<int>(i), p.first, p.second, !forcado});
    }
}

// Conecta cada no aos k vizinhos mais proximos por distancia real (nao por
// ordenacao de eixo). Isso aproxima o grafo de uma malha urbana: cada
// "esquina" so se liga as esquinas vizinhas, nao a todas as outras.
static vector<Aresta> construirGrafoMalha(const vector<Ponto>& nos, int kVizinhos) {
    size_t n = nos.size();
    vector<vector<pair<double, int>>> vizinhosOrdenados(n);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            vizinhosOrdenados[i].push_back({distancia(nos[i], nos[j]), static_cast<int>(j)});
        }
        sort(vizinhosOrdenados[i].begin(), vizinhosOrdenados[i].end());
    }


    set<pair<int, int>> arestasSet;
    for (size_t i = 0; i < n; ++i) {
        int limite = min(kVizinhos, static_cast<int>(vizinhosOrdenados[i].size()));
        for (int v = 0; v < limite; ++v) {
            int j = vizinhosOrdenados[i][v].second;
            arestasSet.insert({min(static_cast<int>(i), j), max(static_cast<int>(i), j)});
        }
    }

    vector<int> pai(n);
    iota(pai.begin(), pai.end(), 0);
    function<int(int)> find = [&](int x) { return pai[x] == x ? x : pai[x] = find(pai[x]); };
    auto unir = [&](int x, int y) { pai[find(x)] = find(y); };
    for (const auto& e : arestasSet) unir(e.first, e.second);

    auto agruparComponentes = [&]() {
        map<int, vector<int>> comp;
        for (size_t i = 0; i < n; ++i) comp[find(static_cast<int>(i))].push_back(static_cast<int>(i));
        return comp;
    };

    auto componentes = agruparComponentes();
    while (componentes.size() > 1) {
        auto it1 = componentes.begin();
        auto it2 = next(it1);
        int melhorA = -1, melhorB = -1;
        double melhorDist = numeric_limits<double>::infinity();
        for (int a : it1->second) {
            for (int b : it2->second) {
                double d = distancia(nos[a], nos[b]);
                if (d < melhorDist) { melhorDist = d; melhorA = a; melhorB = b; }
            }
        }
        arestasSet.insert({min(melhorA, melhorB), max(melhorA, melhorB)});
        unir(melhorA, melhorB);
        componentes = agruparComponentes();
    }

    vector<Aresta> arestas;
    arestas.reserve(arestasSet.size());
    for (const auto& e : arestasSet)
        arestas.push_back({e.first, e.second, distancia(nos[e.first], nos[e.second])});
    return arestas;
}

static void salvarInstancia(const Parametros& args,
                             const Ponto& escola,
                             const vector<Ponto>& paradas,
                             const vector<Ponto>& alunos,
                             const vector<Atribuicao>& atribuicoes,
                             const vector<int>& forcados,
                             const vector<Aresta>& arestas) {
    ofstream f(args.out);
    f.precision(3);
    f << fixed;

    f << "PARAMETROS\n";
    f << "seed " << args.seed << "\n";
    f << "n_paradas " << args.nParadas << "\n";
    f << "n_alunos " << args.nAlunos << "\n";
    f << "n_onibus " << args.nOnibus << "\n";
    f << "capacidade_onibus " << args.capacidade << "\n";
    f << "dispersao " << args.dispersao << "\n";
    f << "modo_alunos " << args.modoAlunos << "\n";
    f << "raio_max_caminhada " << args.raioMaxCaminhada << "\n";
    f << "k_vizinhos " << args.kVizinhos << "\n\n";

    f << "ESCOLA\n";
    f << "0 " << escola.x << " " << escola.y << "\n\n";

    f << "PARADAS id x y\n";
    for (size_t i = 0; i < paradas.size(); ++i) {
        f << (i + 1) << " " << paradas[i].x << " " << paradas[i].y << "\n";
    }
    f << "\n";

    f << "ALUNOS id x y\n";
    for (size_t i = 0; i < alunos.size(); ++i) {
        f << i << " " << alunos[i].x << " " << alunos[i].y << "\n";
    }
    f << "\n";

    // dentro_raio=0 significa atribuicao forcada (aluno sem nenhuma parada
    // dentro de raio_max_caminhada)
    f << "ATRIBUICOES aluno_id parada_id distancia dentro_raio\n";
    for (const auto& a : atribuicoes) {
        f << a.alunoId << " " << (a.paradaId + 1) << " " << a.distancia << " " << (a.dentroRaio ? 1 : 0) << "\n";
    }
    f << "\n";

    f << "ALUNOS_ATRIBUICAO_FORCADA\n";
    for (int id : forcados) f << id << " ";
    f << "\n\n";

    f << "ONIBUS id capacidade\n";
    for (int k = 0; k < args.nOnibus; ++k) f << k << " " << args.capacidade << "\n";
    f << "\n";

    f << "ARESTAS " << arestas.size() << "\n";
    for (const auto& a : arestas) {
        f << a.a << " " << a.b << " " << a.peso << "\n";
    }
}

// pra fica chique
static void imprimirUso() {
    cerr << "Uso: gerador_sbrp [--n_paradas N] [--n_alunos N] [--n_onibus N] "
                 "[--capacidade N] [--largura W] [--altura H] "
                 "[--dispersao uniforme|cluster] [--modo_alunos ancorado|independente] "
                 "[--raio_max_caminhada R] [--k_vizinhos K] [--seed S] [--out arquivo.txt]\n";
}

static Parametros parseArgs(int argc, char** argv) {
    Parametros p;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        auto next = [&](const char* nome) -> string {
            return argv[++i];
        };
        if      (arg == "--n_paradas") p.nParadas = stoi(next("--n_paradas"));
        else if (arg == "--n_alunos") p.nAlunos = stoi(next("--n_alunos"));
        else if (arg == "--n_onibus") p.nOnibus = stoi(next("--n_onibus"));
        else if (arg == "--capacidade") p.capacidade = stoi(next("--capacidade"));
        else if (arg == "--largura") p.largura = stod(next("--largura"));
        else if (arg == "--altura") p.altura = stod(next("--altura"));
        else if (arg == "--dispersao") p.dispersao = next("--dispersao");
        else if (arg == "--modo_alunos") p.modoAlunos = next("--modo_alunos");
        else if (arg == "--raio_max_caminhada") p.raioMaxCaminhada = stod(next("--raio_max_caminhada"));
        else if (arg == "--k_vizinhos") p.kVizinhos = stoi(next("--k_vizinhos"));
        else if (arg == "--seed") p.seed = static_cast<unsigned>(stoul(next("--seed")));
        else if (arg == "--out") p.out = next("--out");
        else if (arg == "--help" || arg == "-h") { imprimirUso(); exit(0); }
        else { cerr << "argumento desconhecido: " << arg << "\n"; imprimirUso(); exit(1); }
    }
    return p;
}

int main(int argc, char** argv) {
    Parametros args = parseArgs(argc, argv);
    mt19937 rng(args.seed);

    Ponto escola;
    if (args.escolaCentralizada) {
        escola = {args.largura / 2.0, args.altura / 2.0};
    } else {
        uniform_real_distribution<double> dx(0.0, args.largura), dy(0.0, args.altura);
        escola = {dx(rng), dy(rng)};
    }


    vector<Ponto> paradas;
    if (args.dispersao == "uniforme") {
        paradas = gerarPontosUniformes(args.nParadas, args.largura, args.altura, rng);
    } else {
        paradas = gerarPontosCluster(args.nParadas, args.largura, args.altura, rng);
    }

    vector<Ponto> alunos;
    if (args.modoAlunos == "independente") {
        // tende a gerar mais atribuicoes forcadas
        alunos = (args.dispersao == "uniforme")
            ? gerarPontosUniformes(args.nAlunos, args.largura, args.altura, rng)
            : gerarPontosCluster(args.nAlunos, args.largura, args.altura, rng);
    } else {
        alunos = gerarAlunosAoRedorDeParadas(args.nAlunos, paradas, args.raioMaxCaminhada,
                                              args.largura, args.altura, rng);
    }

    vector<Atribuicao> atribuicoes;
    vector<int> forcados;
    atribuirAlunosAParadas(alunos, paradas, args.raioMaxCaminhada, atribuicoes, forcados);

    vector<Ponto> nos;
    nos.push_back(escola);
    nos.insert(nos.end(), paradas.begin(), paradas.end());
    auto arestas = construirGrafoMalha(nos, args.kVizinhos);

    salvarInstancia(args, escola, paradas, alunos, atribuicoes, forcados, arestas);

    cout << "Instancia gerada: " << args.out << "\n";
    cout << "  paradas=" << args.nParadas << " alunos=" << args.nAlunos
              << " onibus=" << args.nOnibus << " seed=" << args.seed << "\n";
    if (!forcados.empty()) {
        cout << "  aviso: " << forcados.size()
                  << " aluno(s) sem parada dentro do raio_max_caminhada — atribuidos "
                     "a forca a parada mais proxima (marcados dentro_raio=0) para manter "
                     "a instancia factivel: aumentar --raio_max_caminhada ou --n_paradas\n";
    }
    return 0;
}