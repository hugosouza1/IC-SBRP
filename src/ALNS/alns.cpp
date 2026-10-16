#include "alns.hpp"

static ParametrosALNS parseParametros(int argc, char **argv);


// ---------- distâncias de caminho mínimo (sob demanda) ----------

void DistanciasSobDemanda::inicia(const vector<vector<double>>& g){
    grafo = g;
    quantidadeParadas = g.size();
    distancia.assign(quantidadeParadas, {});
    predecessor.assign(quantidadeParadas, {});
    calculada.assign(quantidadeParadas, 0);
}

const vector<double>& DistanciasSobDemanda::operator[](int origem) const {
    if(!calculada[origem]) dijkstra(origem);
    return distancia[origem];
}

const vector<int>& DistanciasSobDemanda::predecessoresDe(int origem) const {
    if(!calculada[origem]) dijkstra(origem);
    return predecessor[origem];
}

void DistanciasSobDemanda::dijkstra(int origem) const {
    vector<double>& dist = distancia[origem];
    vector<int>& pai = predecessor[origem];

    dist.assign(quantidadeParadas, INFINITO);
    pai.assign(quantidadeParadas, -1);
    dist[origem] = 0;

    priority_queue<pair<double,int>, vector<pair<double,int>>, greater<pair<double,int>>> fila;
    fila.push({0.0, origem});

    while(!fila.empty()){
        auto [du, u] = fila.top(); fila.pop();
        if(du > dist[u]) continue;

        for(int v = 0; v < quantidadeParadas; ++v){
            double peso = grafo[u][v];
            if(peso > 0 && du + peso < dist[v]){
                dist[v] = du + peso;
                pai[v] = u;
                fila.push({dist[v], v});
            }
        }
    }
    calculada[origem] = 1;
}


void ALNS::iniciaDistancias(){
    int n = problema.quantidadeParadas;
    vector<vector<double>> grafo(n, vector<double>(n, 0.0));
    for(int i = 0; i < n; ++i)
        for(int j = 0; j < n; ++j)
            grafo[i][j] = problema.grafoParadas[i][j];
 
    distancias.inicia(grafo);
 
    double soma = 0;
    int contados = 0;
    for(int p = 1; p < problema.quantidadeParadas; ++p){
        if(distancias[0][p] < INFINITO / 2){
            distanciaMaxima = max(distanciaMaxima, 2 * distancias[0][p]);
            soma += distancias[0][p];
            contados++;
        }
    }
    distanciaMedia = contados ? soma / contados : 1;
}


vector<int> ALNS::caminhoReal(int origem, int destino) const {
    const vector<int>& pai = distancias.predecessoresDe(origem);

    vector<int> caminho;
    for(int v = destino; v != -1; v = pai[v]) caminho.push_back(v);
    reverse(caminho.begin(), caminho.end());

    if(caminho.empty() || caminho.front() != origem) return {};
    return caminho;
}

double ALNS::sorteio01(){
    return uniform_real_distribution<double>(0.0, 1.0)(gen);
}

int ALNS::sorteioInteiro(int a, int b){
    return uniform_int_distribution<int>(a, b)(gen);
}


// ---------- utilidades da solução ----------

int ALNS::capacidadeRota(const SolucaoALNS& s, int rota) const {
    return problema.capacidadeOnibus[s.atrOnibusRota[rota]];
}

// depósito -> paradas na ordem -> depósito
double ALNS::distanciaSequencia(const vector<int>& paradas) const {
    if(paradas.empty()) return 0;

    double soma = distancias[0][paradas[0]];
    for(int i = 0; i + 1 < (int)paradas.size(); ++i){
        soma += distancias[paradas[i]][paradas[i+1]];
    }
    soma += distancias[paradas.back()][0];

    return soma;
}

void ALNS::avaliar(SolucaoALNS& s) const {
    double distanciaTotal = 0.0;
    double penalidade = 0.0;

    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(s.alunoPorRota[r] == 0) continue;

        distanciaTotal += s.distanciaRota[r];
        penalidade += PenalidadePorRotaAtiva;

        int estouro = s.alunoPorRota[r] - capacidadeRota(s, r);
        if(estouro > 0) penalidade += PenalidadeEstouroCapacidade * estouro;
    }

    s.fitnessPuro = distanciaTotal;
    s.penalidadeFitness = penalidade;
    s.fitness = distanciaTotal + penalidade;
}

SolucaoALNS ALNS::solucaoVazia(){
    SolucaoALNS s;

    s.atrAlunoParada.assign(problema.quantidadeAlunos, -1);
    s.atrAlunoRota.assign(problema.quantidadeAlunos, -1);
    s.paradasDaRota.assign(quantidadeMaxRota, {});
    s.alunosNaParada.assign(quantidadeMaxRota, vector<int>(problema.quantidadeParadas, 0));
    s.alunoPorRota.assign(quantidadeMaxRota, 0);
    s.distanciaRota.assign(quantidadeMaxRota, 0.0);

    s.atrOnibusRota.assign(quantidadeMaxRota, 0);
    int rotasPorOnibus = max(1, quantidadeMaxRota / problema.quantidadeOnibus);
    for(int r = 0; r < quantidadeMaxRota; ++r){
        s.atrOnibusRota[r] = min(r / rotasPorOnibus, problema.quantidadeOnibus - 1);
    }

    return s;
}

pair<double, int> ALNS::melhorPosicaoInsercao(const vector<int>& paradas, int parada) const {
    double melhorDelta = INFINITO;
    int melhorPos = 0;
    int n = paradas.size();

    for(int pos = 0; pos <= n; ++pos){
        int A = (pos == 0) ? 0 : paradas[pos - 1];
        int B = (pos == n) ? 0 : paradas[pos];

        double delta = distancias[A][parada] + distancias[parada][B] - distancias[A][B];
        if(delta < melhorDelta){
            melhorDelta = delta;
            melhorPos = pos;
        }
    }

    return {melhorDelta, melhorPos};
}

// custo marginal de colocar um aluno na parada X da rota R
CandidatoInsercao ALNS::avaliaInsercao(const SolucaoALNS& s, int parada, int rota, double penalidadeParadaNova) const {
    CandidatoInsercao c;
    c.parada = parada;
    c.rota = rota;

    double custo = 0.0;

    if(s.alunosNaParada[rota][parada] == 0){
        auto [delta, pos] = melhorPosicaoInsercao(s.paradasDaRota[rota], parada);
        custo += delta + penalidadeParadaNova;
        c.posicao = pos;
    }

    if(s.alunoPorRota[rota] == 0) custo += PenalidadePorRotaAtiva;
    if(s.alunoPorRota[rota] + 1 > capacidadeRota(s, rota)) custo += PenalidadeEstouroCapacidade;

    c.custo = custo;
    return c;
}

void ALNS::inserirAluno(SolucaoALNS& s, int aluno, const CandidatoInsercao& c){
    if(s.alunosNaParada[c.rota][c.parada] == 0){
        s.paradasDaRota[c.rota].insert(s.paradasDaRota[c.rota].begin() + c.posicao, c.parada);
        s.distanciaRota[c.rota] = distanciaSequencia(s.paradasDaRota[c.rota]);
    }

    s.alunosNaParada[c.rota][c.parada]++;
    s.alunoPorRota[c.rota]++;
    s.atrAlunoParada[aluno] = c.parada;
    s.atrAlunoRota[aluno] = c.rota;
}

void ALNS::removerAluno(SolucaoALNS& s, int aluno){
    int rota = s.atrAlunoRota[aluno];
    int parada = s.atrAlunoParada[aluno];
    if(rota < 0) return;

    s.alunosNaParada[rota][parada]--;
    s.alunoPorRota[rota]--;

    // era o último aluno da parada: ela sai da sequência
    if(s.alunosNaParada[rota][parada] == 0){
        vector<int>& paradas = s.paradasDaRota[rota];
        paradas.erase(find(paradas.begin(), paradas.end(), parada));
        s.distanciaRota[rota] = distanciaSequencia(paradas);
    }

    s.atrAlunoParada[aluno] = -1;
    s.atrAlunoRota[aluno] = -1;
}

vector<int> ALNS::alunosAtribuidos(const SolucaoALNS& s) const {
    vector<int> alunos;
    for(int a = 0; a < problema.quantidadeAlunos; ++a){
        if(s.atrAlunoRota[a] >= 0) alunos.push_back(a);
    }
    return alunos;
}


// ---------- destruição ----------

vector<int> ALNS::destruicaoAleatoria(SolucaoALNS& s, int quantidade){
    vector<int> alunos = alunosAtribuidos(s);
    shuffle(alunos.begin(), alunos.end(), gen);
    alunos.resize(min((size_t)quantidade, alunos.size()));

    for(int a : alunos) removerAluno(s, a);
    return alunos;
}

// remove as paradas (com todos os alunos) cuja saída mais economiza, por aluno
vector<int> ALNS::destruicaoPior(SolucaoALNS& s, int quantidade){
    vector<pair<double, pair<int,int>>> economias; // (economia por aluno, (rota, parada))

    for(int r = 0; r < quantidadeMaxRota; ++r){
        for(int parada : s.paradasDaRota[r]){
            int membros = s.alunosNaParada[r][parada];
            int cargaDepois = s.alunoPorRota[r] - membros;

            vector<int> semParada = s.paradasDaRota[r];
            semParada.erase(find(semParada.begin(), semParada.end(), parada));

            double economia = s.distanciaRota[r] - distanciaSequencia(semParada);

            int estouro = s.alunoPorRota[r] - capacidadeRota(s, r);
            if(estouro > 0) economia += PenalidadeEstouroCapacidade * min(membros, estouro);
            if(cargaDepois == 0) economia += PenalidadePorRotaAtiva;

            economias.push_back({economia / membros, {r, parada}});
        }
    }

    // embaralha antes pra não desempatar sempre pelo mesmo (rota, parada)
    shuffle(economias.begin(), economias.end(), gen);
    stable_sort(economias.begin(), economias.end(), [](const auto& a, const auto& b){
        return a.first > b.first;
    });

    vector<int> removidos;
    while((int)removidos.size() < quantidade && !economias.empty()){
        int idx = (int)(pow(sorteio01(), 2.0) * economias.size()); // enviesado pros piores
        auto [rota, parada] = economias[idx].second;

        for(int a = 0; a < problema.quantidadeAlunos; ++a){
            if(s.atrAlunoRota[a] == rota && s.atrAlunoParada[a] == parada) removidos.push_back(a);
        }
        economias.erase(economias.begin() + idx);
    }

    for(int a : removidos) removerAluno(s, a);
    return removidos;
}

// todos os alunos de uma (rota, parada) sorteada, até chegar na quantidade
vector<int> ALNS::destruicaoParada(SolucaoALNS& s, int quantidade){
    vector<int> removidos;

    while((int)removidos.size() < quantidade){
        vector<int> alunos = alunosAtribuidos(s);
        if(alunos.empty()) break;

        int sorteado = alunos[sorteioInteiro(0, alunos.size() - 1)];
        int rota = s.atrAlunoRota[sorteado];
        int parada = s.atrAlunoParada[sorteado];

        for(int a : alunos){
            if(s.atrAlunoRota[a] == rota && s.atrAlunoParada[a] == parada){
                removerAluno(s, a);
                removidos.push_back(a);
            }
        }
    }

    return removidos;
}

// esvazia uma rota inteira (50%: a de menor carga; 50%: aleatória)
vector<int> ALNS::destruicaoRota(SolucaoALNS& s, int quantidade){
    vector<int> rotasAbertas;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(s.alunoPorRota[r] > 0) rotasAbertas.push_back(r);
    }
    if(rotasAbertas.empty()) return {};

    int rota = rotasAbertas[sorteioInteiro(0, rotasAbertas.size() - 1)];
    if(sorteio01() < 0.5){
        rota = *min_element(rotasAbertas.begin(), rotasAbertas.end(), [&](int x, int y){
            return s.alunoPorRota[x] < s.alunoPorRota[y];
        });
    }

    vector<int> removidos;
    for(int a = 0; a < problema.quantidadeAlunos; ++a){
        if(s.atrAlunoRota[a] == rota) removidos.push_back(a);
    }

    for(int a : removidos) removerAluno(s, a);
    return removidos;
}

// Shaw: uma semente + alunos parecidos (paradas próximas e/ou mesma rota)
vector<int> ALNS::destruicaoRelacionada(SolucaoALNS& s, int quantidade){
    vector<int> alunos = alunosAtribuidos(s);
    if(alunos.empty()) return {};

    int semente = alunos[sorteioInteiro(0, alunos.size() - 1)];

    vector<pair<double,int>> relacao;

    for(int a : alunos){
        if(a == semente) continue;

        double r = distancias[s.atrAlunoParada[semente]][s.atrAlunoParada[a]] / distanciaMaxima + (s.atrAlunoRota[semente] == s.atrAlunoRota[a] ? 0.0 : 0.5);
        relacao.push_back({r, a});
    }

    sort(relacao.begin(), relacao.end()); // parecidos prox de 0

    vector<int> removidos = {semente};
    while((int)removidos.size() < quantidade && !relacao.empty()){
        int idx = (int)(pow(sorteio01(), 6.0) * relacao.size()); // enviesado pros mais parecidos
        removidos.push_back(relacao[idx].second);
        relacao.erase(relacao.begin() + idx);
    }

    for(int a : removidos) removerAluno(s, a);
    return removidos;
}


// ---------- reparo ----------
// gulosa (k = 1), regret-k (k >= 2), consolidadora (penalidade de parada nova), com ruído

void ALNS::reparar(SolucaoALNS& s, vector<int> pendentes, const ConfigReparo& config){
    uniform_real_distribution<double> ruido(1.0 - config.amplitudeRuido, 1.0 + config.amplitudeRuido);
    
    double penalidadeParadaNova = config.fatorPenalidadeParadaNova * distanciaMedia;

    while(!pendentes.empty()){

        int escolhido = -1;
        double melhorPrioridade = -INFINITO;
        CandidatoInsercao melhorCandidato;

        for(int idx = 0; idx < (int)pendentes.size(); ++idx){
            int aluno = pendentes[idx];

            // melhor candidato (parada, posição) de cada rota
            vector<CandidatoInsercao> candidatosPorRota(quantidadeMaxRota);

            for(const auto& par : problema.alunosParadas[aluno].paradasPossiveis){
                for(int r = 0; r < quantidadeMaxRota; ++r){
                    CandidatoInsercao c = avaliaInsercao(s, par.first, r, penalidadeParadaNova);

                    if(config.amplitudeRuido > 0 && c.custo > 0) c.custo *= ruido(gen);
                    if(c.custo < candidatosPorRota[r].custo) candidatosPorRota[r] = c;
                }
            }

            vector<CandidatoInsercao> viaveis;
            for(const auto& c : candidatosPorRota){
                if(c.custo < INFINITO / 2) viaveis.push_back(c);
            }
            
            if(viaveis.empty()) continue;

            sort(viaveis.begin(), viaveis.end(), [](const CandidatoInsercao& a, const CandidatoInsercao& b){
                return a.custo < b.custo;
            });

            double prioridade;
            if(config.regretK <= 1){
                prioridade = -viaveis[0].custo; // gulosa: menor custo primeiro
            } else {
                double arrependimento = 0.0;    // regret-k: quem mais perde se esperar
                for(int j = 1; j < config.regretK; ++j){
                    arrependimento += (j < (int)viaveis.size() ? viaveis[j].custo : 1e6) - viaveis[0].custo;
                }
                prioridade = arrependimento - 1e-6 * viaveis[0].custo; // desempate pelo menor custo
            }

            if(prioridade > melhorPrioridade){
                melhorPrioridade = prioridade;
                escolhido = idx;
                melhorCandidato = viaveis[0];
            }
        }

        if(escolhido < 0){
            cerr << "AVISO: aluno sem candidato viavel\n";
            break;
        }

        inserirAluno(s, pendentes[escolhido], melhorCandidato);
        pendentes.erase(pendentes.begin() + escolhido);
    }
}


// ---------- pós-reparo ----------

void ALNS::buscaLocal2Opt(SolucaoALNS& s, int rota){
    vector<int>& paradas = s.paradasDaRota[rota];
    int n = paradas.size();
    if(n < 3) return;

    double distanciaAtual = distanciaSequencia(paradas);
    bool melhorou = true;

    int maximo = 100;
    int it = 0;

    while(melhorou && maximo > it){
        melhorou = false;

        for(int i = 0; i < n - 1; ++i){
            for(int j = i + 1; j < n; ++j){
                reverse(paradas.begin() + i, paradas.begin() + j + 1);

                double nova = distanciaSequencia(paradas);
                if(nova < distanciaAtual - 1e-9){
                    distanciaAtual = nova;
                    melhorou = true;
                } else {
                    reverse(paradas.begin() + i, paradas.begin() + j + 1); // desfaz
                }
            }
        }
        ++it;
    }

    s.distanciaRota[rota] = distanciaAtual;
}


// troca de ônibus entre rotas (resolve estouro de capacidade)
void ALNS::trocaOnibusEntreRotas(SolucaoALNS& s){
    avaliar(s);

    for(int r1 = 0; r1 < quantidadeMaxRota; ++r1){
        for(int r2 = r1 + 1; r2 < quantidadeMaxRota; ++r2){
            if(s.atrOnibusRota[r1] == s.atrOnibusRota[r2]) continue;

            double antes = s.fitness;
            swap(s.atrOnibusRota[r1], s.atrOnibusRota[r2]);
            avaliar(s);

            if(s.fitness >= antes - 1e-9){
                swap(s.atrOnibusRota[r1], s.atrOnibusRota[r2]); // desfaz
                avaliar(s);
            }
        }
    }
}


// ---------- operadores e solução inicial ----------

void ALNS::registraOperadores(){
    destruicoes = {
        {"aleatoria",   [this](SolucaoALNS& s, int q){ return destruicaoAleatoria(s, q); }}, // aleatorio
        {"pior",        [this](SolucaoALNS& s, int q){ return destruicaoPior(s, q); }}, // remove alunos de paradas are a quantidade 
        {"parada",      [this](SolucaoALNS& s, int q){ return destruicaoParada(s, q); }}, // escolhe uma parada e rota para tirar
        {"rota",        [this](SolucaoALNS& s, int q){ return destruicaoRota(s, q); }}, // rota inteira
        {"relacionada", [this](SolucaoALNS& s, int q){ return destruicaoRelacionada(s, q); }}, // remove os mais parecidos
    };

    //            nome    /  k  / ruido  / penalidade parada nova
    reparos = {
        {"gulosa",       1, 0.0, 0.0},
        {"regret2",      2, 0.0, 0.0},
        {"regret3",      3, 0.0, 0.0},
        {"consolidada",  2, 0.0, 0.5},
        {"ruido",        1, 0.2, 0.0},
    };
}

SolucaoALNS ALNS::construirSolucaoInicial(){
    SolucaoALNS s = solucaoVazia();

    vector<int> todos(problema.quantidadeAlunos);
    iota(todos.begin(), todos.end(), 0);

    reparar(s, todos, reparos[1]); // regret-2

    for(int r = 0; r < quantidadeMaxRota; ++r) buscaLocal2Opt(s, r);

    trocaOnibusEntreRotas(s);
    avaliar(s);

    return s;
}


// ---------- laço principal ----------

SolucaoALNS ALNS::executar(){
    SolucaoALNS atual = construirSolucaoInicial();
    SolucaoALNS melhor = atual;

    double temperatura = FracaoTemperaturaInicial * atual.fitness / std::log(2.0);

    int nDestruicoes = destruicoes.size();
    int nReparos = reparos.size();

    vector<double> pesosDestruicao(nDestruicoes, 1.0), pesosReparo(nReparos, 1.0);
    vector<double> pontosDestruicao(nDestruicoes, 0.0), pontosReparo(nReparos, 0.0);
    vector<int> usosDestruicao(nDestruicoes, 0), usosReparo(nReparos, 0);

    auto inicio = steady_clock::now();
    auto decorrido = [&](){ return duration<double>(steady_clock::now() - inicio).count(); };

    for(long it = 1; it <= MaxIteracoes && decorrido() < TempoMaxSegundos; ++it){

        int d = discrete_distribution<int>(pesosDestruicao.begin(), pesosDestruicao.end())(gen);
        int r = discrete_distribution<int>(pesosReparo.begin(), pesosReparo.end())(gen);

        SolucaoALNS nova = atual;

        int minimo = max(1, (int)(FracaoMinRemocao * problema.quantidadeAlunos));
        int maximo = max(minimo, (int)(FracaoMaxRemocao * problema.quantidadeAlunos));
        int quantidade = sorteioInteiro(minimo, maximo);

        vector<int> removidos = destruicoes[d].second(nova, quantidade);
        reparar(nova, removidos, reparos[r]);

        // 2-opt só nas rotas que mudaram
        for(int rota = 0; rota < quantidadeMaxRota; ++rota){
            if(nova.paradasDaRota[rota] != atual.paradasDaRota[rota]) buscaLocal2Opt(nova, rota);
        }

        trocaOnibusEntreRotas(nova);
        avaliar(nova);

        bool aceita = nova.fitness < atual.fitness - 1e-9
                   || sorteio01() < exp(-(nova.fitness - atual.fitness) / max(temperatura, 1e-12));

        double pontos = 0.0;
        if(nova.fitness < melhor.fitness - 1e-9){
            melhor = nova;
            pontos = RecompensaNovoMelhor;
        }
        else if(nova.fitness < atual.fitness - 1e-9) pontos = RecompensaMelhora;
        else if(aceita) pontos = RecompensaAceito;

        if(aceita) atual = nova;

        pontosDestruicao[d] += pontos; usosDestruicao[d]++;
        pontosReparo[r] += pontos;     usosReparo[r]++;

        // fim do segmento: atualiza os pesos
        if(it % TamanhoSegmento == 0){
            for(int i = 0; i < nDestruicoes; ++i){
                if(usosDestruicao[i]){
                    pesosDestruicao[i] = (1 - TaxaAprendizado) * pesosDestruicao[i]
                                       + TaxaAprendizado * pontosDestruicao[i] / usosDestruicao[i];
                }
                pesosDestruicao[i] = max(pesosDestruicao[i], 0.01);
                pontosDestruicao[i] = 0; usosDestruicao[i] = 0;
            }
            for(int i = 0; i < nReparos; ++i){
                if(usosReparo[i]){
                    pesosReparo[i] = (1 - TaxaAprendizado) * pesosReparo[i]
                                   + TaxaAprendizado * pontosReparo[i] / usosReparo[i];
                }
                pesosReparo[i] = max(pesosReparo[i], 0.01);
                pontosReparo[i] = 0; usosReparo[i] = 0;
            }

            if(log){
                cerr << "it " << it << " t=" << fixed << setprecision(1) << decorrido()
                     << " melhor=" << melhor.fitness << " atual=" << atual.fitness
                     << " T=" << temperatura << "\n  destruicao:";
                for(int i = 0; i < nDestruicoes; ++i)
                    cerr << " " << destruicoes[i].first << "=" << setprecision(2) << pesosDestruicao[i];
                cerr << "\n  reparo:    ";
                for(int i = 0; i < nReparos; ++i)
                    cerr << " " << reparos[i].nome << "=" << pesosReparo[i];
                cerr << "\n";
            }
        }

        temperatura *= TaxaResfriamento;
    }

    return melhor;
}


void ALNS::imprimeSolucao(SolucaoALNS& sol){
    int rotasAbertas = 0, estouro = 0;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(sol.alunoPorRota[r] > 0){
            rotasAbertas++;
            estouro += max(0, sol.alunoPorRota[r] - capacidadeRota(sol, r));
        }
    }

    cout << "\n========================================\n";
    cout << "        SOLUCAO - ALNS (SBRP)\n";
    cout << "========================================\n\n";
    cout << "Distancia total: " << sol.fitnessPuro << " [penalidade:" << sol.penalidadeFitness << "]" << "\n";
    cout << "Rotas abertas: " << rotasAbertas << " | estouro de capacidade: " << estouro << "\n";

    int rotaImpressa = 0;
    for(int r = 0; r < quantidadeMaxRota; ++r){
        if(sol.alunoPorRota[r] == 0) continue;

        cout << "\n\n---- Rota " << rotaImpressa << " ----\n";
        cout << "Ônibus atribuído: " << sol.atrOnibusRota[r] << "(capacidade: " << capacidadeRota(sol, r) << ")\n";
        cout << "Peso da rota: " << sol.distanciaRota[r] << "\n";

        cout << "Paradas: 0";
        for(int p : sol.paradasDaRota[r]) cout << " -> " << p;
        cout << " -> 0\n";

        // caminho real (expandido pelos predecessores do Dijkstra)
        vector<int> alvo = sol.paradasDaRota[r];
        alvo.push_back(0);

        vector<int> trajeto = {0};
        int atual = 0;
        for(int p : alvo){
            vector<int> trecho = caminhoReal(atual, p);
            if(trecho.empty()) continue;
            trajeto.insert(trajeto.end(), trecho.begin() + 1, trecho.end());
            atual = p;
        }

        cout << "Trajeto (paradas): ";
        for(int i = 0; i < (int)trajeto.size(); i++){
            cout << trajeto[i];
            if(i + 1 < (int)trajeto.size()) cout << " -> ";
        }
        cout << "\n";

        cout << "Estudantes (" << sol.alunoPorRota[r] << "): [parada->alunos]\n";
        for(int parada : sol.paradasDaRota[r]){
            cout << "\t\t[" << parada << " -> ";
            bool primeiro = true;
            for(int aluno = 0; aluno < problema.quantidadeAlunos; ++aluno){
                if(sol.atrAlunoRota[aluno] == r && sol.atrAlunoParada[aluno] == parada){
                    if(!primeiro) cout << ", ";
                    cout << aluno;
                    primeiro = false;
                }
            }
            cout << "]\n";
        }

        rotaImpressa++;
    }

    cout << "\n----------------------------------------\n";
    cout << "Total de onibus usados: " << rotaImpressa << "\n";
    cout << "Distancia total: " << sol.fitnessPuro << " [penalidade:" << sol.penalidadeFitness << "]" << "\n";
    cout << "========================================\n";
}


int main(int argc, char *argv[]){
    if(argc < 2){
        cerr << "ERRO: falta o arquivo de instancia\n";
        return 1;
    }

    infoSBRP dados(argv[1]);

    ParametrosALNS p = parseParametros(argc, argv);

    ALNS alns(dados, p);

    SolucaoALNS melhor = alns.executar();

    if(p.log) alns.imprimeSolucao(melhor);
    else cout << melhor.fitnessPuro << "\n";

    return 0;
}


ParametrosALNS parseParametros(int argc, char** argv) {

    ParametrosALNS p;

    for (int i = 2; i < argc; ++i) {

        string arg = argv[i];

        auto next = [&](const char* nome) -> string {
            if (i + 1 >= argc) {
                cerr << "Parametro ausente para " << nome << "\n";
                exit(1);
            }

            return argv[++i];
        };

        if(arg == "--seed")
            p.seed = stoi(next("--seed"));

        else if (arg == "--log")
            p.log = "true" == next("--log");

        // Orçamento
        else if (arg == "--tempo")
            p.tempoMaxSegundos = stod(next("--tempo"));

        else if (arg == "--max_iteracoes")
            p.maxIteracoes = stol(next("--max_iteracoes"));

        // Destruição
        else if (arg == "--fracao_min_remocao")
            p.fracaoMinRemocao = stod(next("--fracao_min_remocao"));

        else if (arg == "--fracao_max_remocao")
            p.fracaoMaxRemocao = stod(next("--fracao_max_remocao"));

        // Aprendizado
        else if (arg == "--tamanho_segmento")
            p.tamanhoSegmento = stoi(next("--tamanho_segmento"));

        else if (arg == "--taxa_aprendizado")
            p.taxaAprendizado = stod(next("--taxa_aprendizado"));

        else if (arg == "--recompensa_novo_melhor")
            p.recompensaNovoMelhor = stod(next("--recompensa_novo_melhor"));

        else if (arg == "--recompensa_melhora")
            p.recompensaMelhora = stod(next("--recompensa_melhora"));

        else if (arg == "--recompensa_aceito")
            p.recompensaAceito = stod(next("--recompensa_aceito"));

        // Aceitação
        else if (arg == "--temperatura_inicial")
            p.fracaoTemperaturaInicial = stod(next("--temperatura_inicial"));

        else if (arg == "--resfriamento")
            p.taxaResfriamento = stod(next("--resfriamento"));

        // Penalidades
        else if (arg == "--penalidade_rota")
            p.penalidadePorRotaAtiva = stod(next("--penalidade_rota"));

        else if (arg == "--penalidade_estouro")
            p.penalidadeEstouroCapacidade = stod(next("--penalidade_estouro"));
    }

    return p;
}