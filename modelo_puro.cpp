#include <iostream>
#include <fstream>
#include <vector>
#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>

using namespace std;
ILOSTLBEGIN //MACRO - "using namespace" for ILOCPEX

//CPLEX Parameters
#define CPLEX_TIME_LIM 3600 //3600 segundos
int INF = INT_MAX;
//#define CPLEX_COMPRESSED_TREE_MEM_LIM 8128 //8GB
//#define CPLEX_WORK_MEM_LIM 4096 //4GB
//#define CPLEX_VARSEL_MODE 0
/*
* VarSel Modes:
* -1 Branch on variable with minimum infeasibility
* 0 Branch variable automatically selected
* 1 Branch on variable with maximum infeasibility
* 2 Branch based on pseudo costs
* 3 Strong branching
* 4 Branch based on pseudo reduced costs
*
* Default: 0
*/

struct estudante{
    int id; // estudante A

    // distancia
    vector<pair<int, int>> paradasPossiveis;
};

class infoSBRP{
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
	    void cplex();
};

void infoSBRP::leitura(string arquivoEntrada){
    ifstream arq(arquivoEntrada);
    if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1);}

    arq >> quantidadeParadas >> quantidadeAlunos >> quantidadeOnibus >> Q;

    quantidadeRotas = ((quantidadeAlunos + Q) / Q) + 2; // caiu de 4min pra 30s

    quantidadePassos = quantidadeParadas; // de 30 do de cima pra 9s. diliça
    quantidadePassos = quantidadeParadas * quantidadeOnibus * 2; // de 30 do de cima pra 9s. diliça

    grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, INF));
    for(int i = 0; i < quantidadeParadas; i++){
        grafoParadas[i][i] = 0;
    }

    arq >> quantidadeArestas;
    for(int i = 0; i < quantidadeArestas; i++){
        int pontoA, pontoB, peso; 
        arq >> pontoA >> pontoB >> peso;
        grafoParadas[pontoA][pontoB] = min(grafoParadas[pontoA][pontoB], peso);
        grafoParadas[pontoB][pontoA] = min(grafoParadas[pontoB][pontoA], peso);
    }


    alunosParadas.clear();
    maxDistancia = 0;
    for(int e = 0; e < quantidadeAlunos; e++){
        estudante aluno;
        int quantidadeParadasPossiveis;

        /*
            exemplo:

            0 2
            1 10
            3 15

            aluno 0
            possui 2 paradas possíveis
        */

        arq >> aluno.id;
        arq >> quantidadeParadasPossiveis;

        for(int j = 0; j < quantidadeParadasPossiveis; j++){
            int parada;
            int distancia;

            arq >> parada >> distancia;

            aluno.paradasPossiveis.push_back({ parada, distancia });

            if(distancia > maxDistancia){
                maxDistancia = distancia;
            }
        }

        alunosParadas.push_back(aluno);
    }

    arq.close();
}


void infoSBRP::cplex(){
	try{
       //CPLEX
	IloEnv env; //Define o ambiente do CPLEX

	//Variaveis --------------------------------------------- 
	int numberVar = 0; //Total de Variaveis
	int numberRes = 0; //Total de Restricoes


	//---------- MODELAGEM ---------------
	
	// Variavel de decisão W
	IloNumVar W(env, 0, IloInfinity, ILOFLOAT);
	numberVar++;

	IloNumVar M(env, 0, IloInfinity, ILOINT);
	numberVar++;
	
	// ======= VARIAVEIS DE DECISAO (x_i) binaria ==========

	// b
    	IloNumVarArray b(env);
	for(int i = 0; i < quantidadeParadas; i++ ){
        b.add(IloIntVar(env, 0, 1));
		numberVar++;
	}

	// z
	IloNumVarArray z(env);
	for(int i = 0; i < quantidadeOnibus; i++ ){
        z.add(IloIntVar(env, 0, 1));
		numberVar++;
	}

	
	
	// =========== Variaveis de Decisao 2 dimensoes (x_ij) binarias ===========
	
	// a_ei
	IloArray<IloNumVarArray> a(env);
	
	for(int e = 0; e < quantidadeAlunos; e++){
		a.add(IloNumVarArray(env));
		
	    for(auto p : alunosParadas[e].paradasPossiveis){
			a[e].add(IloIntVar(env, 0, 1));
	        numberVar++;
	    }
	}
	
	//  tr^k  (rota r do onibus k esta ativa)
	IloArray<IloNumVarArray> t(env);
	
	for(int k = 0; k < quantidadeOnibus; k++){
		t.add(IloNumVarArray(env));
		
		for(int r = 0; r < quantidadeRotas; r++){
			t[k].add(IloIntVar(env, 0, 1));
			numberVar++;
		}
	}

	// x^kr-st_ij : arco i->j usado no passo "st" da rota r do onibus k.
	IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>> x(env);
	for(int k = 0; k < quantidadeOnibus; k++){
		x.add(IloArray<IloArray<IloArray<IloNumVarArray>>>(env));

		for(int r = 0; r < quantidadeRotas; r++){
			x[k].add(IloArray<IloArray<IloNumVarArray>>(env));

			for(int st = 0; st < quantidadePassos; st++){
				x[k][r].add(IloArray<IloNumVarArray>(env));

				for(int i = 0; i < quantidadeParadas; i++){
					x[k][r][st].add(IloNumVarArray(env));

					for(int j = 0; j < quantidadeParadas; j++){

                        // aresta inexistente, alto-cilco, menos na origem. precisa de loop na escola pra consumir o step
						if((grafoParadas[i][j] == INF) || (i == j && i != 0)){
							x[k][r][st][i].add(IloIntVar(env, 0, 0)); // Trava em 0
						}
						else{
							x[k][r][st][i].add(IloIntVar(env, 0, 1)); // Variável binária normal
						}

						numberVar++;
					}
				}
			}
		}
	}

	// p_i^kr  (parada i visitada pela rota r do onibus k)
    IloArray<IloArray<IloNumVarArray>> p(env);
    for(int k = 0; k < quantidadeOnibus; k++){
        p.add(IloArray<IloNumVarArray>(env));

        for(int r = 0; r < quantidadeRotas; r++){
            p[k].add(IloNumVarArray(env));

            for(int i = 0; i < quantidadeParadas; i++){
                p[k][r].add(IloIntVar(env, 0, 1));
                numberVar++;
            }
        }
    }

	// y_ei^kr
	IloArray<IloArray<IloArray<IloNumVarArray>>> y(env);
	for(int k = 0; k < quantidadeOnibus; k++){
	    y.add(IloArray<IloArray<IloNumVarArray>>(env));

	    for(int r = 0; r < quantidadeRotas; r++){
	        y[k].add(IloArray<IloNumVarArray>(env));

	        for(int e = 0; e < quantidadeAlunos; e++){
	            y[k][r].add(IloNumVarArray(env));

	            for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	                y[k][r][e].add(IloIntVar(env,0,1));
	            }
	        }
	    }
	}
    
	//Definicao do ambiente modelo --------------------------------
	IloModel model ( env );
	
	//FUNCAO OBJETIVO ---------------------------------------------
	IloExpr obj1(env); // custo
	IloExpr obj2(env); // qtd paradas

	// Restrição 1 (custo) - com step
    for(int k = 0; k < quantidadeOnibus; k++){
		for(int r = 0; r < quantidadeRotas; r++ ){
			for(int st = 0; st < quantidadePassos; st++){
				for(int i = 0; i < quantidadeParadas; i++){
					for(int j = 0;  j < quantidadeParadas; j++){
						obj1 += (grafoParadas[i][j] * x[k][r][st][i][j]);
					}
				}
			}
        }
	}

    // Restrição 2 (quantidade de parada)
    for(int i = 0;  i < quantidadeParadas; i++){
		obj2 += (b[i]);
    }
	
	//RESTRICOES ---------------------------------------------	

    IloExpr soma(env); 

    // --- ALOCAÇÃO DE ESTUDANTES ÀS PARADAS ---
    
    // Cada estudante deve ser alocado a exatamente uma parada valida
    for(int e = 0; e < quantidadeAlunos; e++) {
        soma.clear();
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            soma += a[e][p];
        }
        model.add(soma == 1);
        numberRes++;
    }

    // Um estudante só pode ser alocado a uma parada se ela estiver ativa
    for(int e = 0; e < quantidadeAlunos; e++) {
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = alunosParadas[e].paradasPossiveis[p].first;
            model.add(a[e][p] <= b[i]);
            numberRes++;
        }
    }

    // Define W como a maior distância de caminhada
    for(int e = 0; e < quantidadeAlunos; e++) {
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = alunosParadas[e].paradasPossiveis[p].first;
            double d_ei = alunosParadas[e].paradasPossiveis[p].second;
            model.add(d_ei * a[e][p] <= W);
            numberRes++;
        }
    }

    // ======== CONSERVAÇÃO DE FLUXO E ROTEAMENTO (Step) ========
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {

            // Encadeamento entre passos consecutivos: (nó de chegada do passo st) == (no de saida do passo st+1), para cada nó j
            for(int st = 0; st < quantidadePassos - 1; st++) {
                for(int j = 0; j < quantidadeParadas; j++) {
                    IloExpr chegadaEm_j_no_passo_st(env);
                    IloExpr saidaDe_j_no_passo_stMais1(env);

                    for(int i = 0; i < quantidadeParadas; i++) {
                        chegadaEm_j_no_passo_st += x[k][r][st][i][j];
                    }
                    for(int i = 0; i < quantidadeParadas; i++) {
                        saidaDe_j_no_passo_stMais1 += x[k][r][st + 1][j][i];
                    }

                    model.add(chegadaEm_j_no_passo_st == saidaDe_j_no_passo_stMais1);
                    numberRes++;

                    chegadaEm_j_no_passo_st.end();
                    saidaDe_j_no_passo_stMais1.end();
                }
            }

            // No maximo um arco ativo por passo: soma de todos os arcos do passo st <= 1
            for(int st = 0; st < quantidadePassos; st++) {
                soma.clear();
                for(int i = 0; i < quantidadeParadas; i++) {
                    for(int j = 0; j < quantidadeParadas; j++) {
                        soma += x[k][r][st][i][j];
                    }
                }
                model.add(soma <= 1);
                numberRes++;
            }

            // O self-loop em 0 (x[k][r][st][0][0]) fica de fora dessa soma de proposito: 
            // ele representa a rota inativa/ociosa, entao nao pode ser limitado por t
            for(int st = 0; st < quantidadePassos; st++) {
                soma.clear();
                for(int i = 0; i < quantidadeParadas; i++) {
                    for(int j = 0; j < quantidadeParadas; j++) {
                        if(i == 0 && j == 0) continue; 
                        soma += x[k][r][st][i][j];
                    }
                }
                model.add(soma <= t[k][r]);
                numberRes++;
            }

            // Toda rota ativa deve sair "de verdade" da escola (0) no
            // passo 0 (arco 0->j, j!=0) exatamente quando t=1; se a
            // rota estiver inativa, o passo 0 fica em self-loop (0->0)
            // representando que nunca saiu
            soma.clear();
            for(int j = 1; j < quantidadeParadas; j++) {
                soma += x[k][r][0][0][j];
            }

            model.add(soma == t[k][r]);
            numberRes++;

            model.add(x[k][r][0][0][0] == 1 - t[k][r]);
            numberRes++;

            // A escola só pode ser ponto de partida real no passo 0.
            // Nos demais passos, se a rota ja voltou, ela deve permanecer em self-loop
            for(int st = 1; st < quantidadePassos; st++){
                soma.clear();
                for(int j = 1; j < quantidadeParadas; j++){
                    soma += x[k][r][st][0][j];
                }
                model.add(soma == 0);
                numberRes++;
            }

            // A rota deve retornar a escola (0) pelo menos uma vez, vindo de um no diferente de 0 (chegada de verdade, não self-loop), em algum passo
            soma.clear();
            for(int st = 0; st < quantidadePassos; st++) {
                for(int i = 1; i < quantidadeParadas; i++) {
                    soma += x[k][r][st][i][0];
                }
            }
            model.add(soma == t[k][r]); 
            numberRes++;
        }
    }

    // ============= ATIVAÇÃO E ASSOCIAÇÃO DE PARADAS =============

    // Vincula s[k][r][i] a "a parada i foi visitada em algum passo".
    // Como agora pode haver mais de uma entrada em i (revisita):
    //   soma(entradas em i, todos os passos) <= quantidadePassos * s
    //   s <= soma(entradas em i, todos os passos)
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            for(int i = 0; i < quantidadeParadas; i++) {
                soma.clear();
                for(int st = 0; st < quantidadePassos; st++) {
                    for(int j = 0; j < quantidadeParadas; j++) {
                        soma += x[k][r][st][j][i];
                    }
                }
                model.add(soma <= quantidadePassos * p[k][r][i]);
                numberRes++;

                model.add(p[k][r][i] <= soma);
                numberRes++;
            }
        }
    }

    // Se a parada b_i está ativa, pelo menos uma rota deve visitá-la
    for(int i = 0; i < quantidadeParadas; i++) {
        soma.clear();
        for(int k = 0; k < quantidadeOnibus; k++) {
            for(int r = 0; r < quantidadeRotas; r++) {
                soma += p[k][r][i];
            }
        }
        model.add(soma >= b[i]);
        numberRes++;
    }

    // ============= CAPACIDADE DOS VEÍCULOS =============

    // Limita a quantidade de estudantes atendidos na rota à capacidade máxima Q 
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            soma.clear();
            for(int e = 0; e < quantidadeAlunos; e++) {
                for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
                    soma += y[k][r][e][p];
                }
            }
            
            model.add(soma <= Q * t[k][r]); 
            numberRes++;
        }
    }
    
    // ============= LINEARIZAÇÃO DA VARIÁVEL AUXILIAR Y (y = a * s) =============

    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            for(int e = 0; e < quantidadeAlunos; e++) {
                for(int pp = 0; pp < alunosParadas[e].paradasPossiveis.size(); pp++) {
                    int i = alunosParadas[e].paradasPossiveis[pp].first;

                    // y <= a
                    model.add(y[k][r][e][pp] <= a[e][pp]);
                    
                    // y <= s
                    model.add(y[k][r][e][pp] <= p[k][r][i]);
                    
                    // y >= a + s - 1
                    model.add(y[k][r][e][pp] >= a[e][pp] + p[k][r][i] - 1);
                    
                    numberRes += 3;
                }
            }
        }
    }

    // ============= RELAÇÃO ÔNIBUS-ROTA =============

    // Ativa z_k se o ônibus k operar ao menos uma rota
    for(int k = 0; k < quantidadeOnibus; k++) {
        soma.clear();
        for(int r = 0; r < quantidadeRotas; r++) {
            soma += t[k][r];
        }
        model.add(soma >= z[k]);
        numberRes++;
    }

    // Impede rotas em ônibus desativados
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            model.add(t[k][r] <= z[k]);
            numberRes++;
        }
    }
    

    // essa restrição ta quebrando o balanceamento. Depois olhar com
    // carinho pra achar o pq 
    // // Cada rota  r só pode pertencer a no máximo um veículo k
    // for(int r = 0; r < quantidadeRotas; r++) {
    //     soma.clear();
    //     for(int k = 0; k < quantidadeOnibus; k++) {
    //         soma += t[k][r];
    //     }
    //     model.add(soma <= 1);
    //     numberRes++;
    // }
    

    // Força o uso das rotas em ordem: rota r -> rota r+1
    // for(int k = 0; k < quantidadeOnibus; k++){
    //     for(int r = 1; r < quantidadeRotas; r++){
    //         model.add(t[k][r] <= t[k][r - 1]);
    //         numberRes++;
    //     }
    // }

    // 2.0 rota global
    for(int r = 1; r < quantidadeRotas; r++){
        IloExpr usoAtual(env);
        IloExpr usoAnterior(env);

        for(int k = 0; k < quantidadeOnibus; k++){
            usoAtual    += t[k][r];
            usoAnterior += t[k][r - 1];
        }

        model.add(usoAtual <= usoAnterior);
        numberRes++;

        usoAtual.end();
        usoAnterior.end();
    }

    // Força o uso dos onibus em ordem: oni k -> oni k+1
    for(int k = 1; k < quantidadeOnibus; k++){
        model.add(z[k] <= z[k - 1]);
        numberRes++;
    }



    // ============= BALANCEAMENTO =============

    // M
    for(int k = 0; k < quantidadeOnibus; k++) {
        soma.clear();
        for(int r = 0; r < quantidadeRotas; r++) {
            soma += t[k][r];
        }
        model.add(M >= soma);
        numberRes++;
    }

    soma.end();
	
	//------ EXECUCAO do MODELO ----------
    // time_t timer, timer2;
    IloNum objValue;
    double runTime;
    string status;
    
    printf("--------Informacoes da Execucao:----------\n\n");
    printf("#Var: %d\n", numberVar);
    printf("#Restricoes: %d\n", numberRes);
    cout << "Memory usage after variable creation:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
    
    IloCplex cplex(env); // Inicialize apontando para o ambiente
    cplex.extract(model); 
    
    cout << "Memory usage after cplex extraction:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;

    cplex.setParam(IloCplex::TiLim, CPLEX_TIME_LIM);
    
    time_t tInicio, tFimFase1, tFimFase2, tFimFase3, tFimFase4;


    // time(&timer);


    // ==============
    time(&tInicio);
    // ==============

    // ================= FASE 1: Minimizar Custo (obj1) =================
    IloObjective FO1 = IloMinimize(env, obj1);
    model.add(FO1);
    cplex.extract(model); // Atualiza o modelo no solver
    if (!cplex.solve()) {
        cerr << "Erro: Fase 1 inviável!\n";
        return;
    }
	
    double melhorCusto = cplex.getObjValue();
    model.remove(FO1);
    model.add(obj1 <= melhorCusto + 1e-4); 
    
    // ==============
    time(&tFimFase1);
    // ==============

    // ================= FASE 2: Minimizar Qtd Paradas (obj2) =================
    IloObjective FO2 = IloMinimize(env, obj2);
    model.add(FO2);
    cplex.extract(model);
    if (!cplex.solve()) {
        cerr << "Erro: Fase 2 inviável!\n";
        return;
    }
    double melhorQtdParadas = cplex.getObjValue();
    model.remove(FO2);
    model.add(obj2 <= melhorQtdParadas + 1e-4);

    // ==============
    time(&tFimFase2);
    // ==============

    // ================= FASE 3: Minimizar maior caminhada (W) =================
    IloObjective FO3 = IloMinimize(env, W);
    model.add(FO3);
    cplex.extract(model);   
    if (!cplex.solve()) {
        cerr << "Erro: Fase 3 inviável!\n";
        return;
    }
    double melhorDistW = cplex.getObjValue();
    model.remove(FO3); 
    model.add(W <= melhorDistW + 1e-4);

    // ==============
    time(&tFimFase3);
    // ==============


    // ================= FASE 4: Minimizar Max Rotas (M) =================
    IloObjective FO4 = IloMinimize(env, M);
    model.add(FO4);
    cplex.extract(model);
    cplex.solve();
    
    // time(&timer2);

    // ==============
    time(&tFimFase4);
    // ==============

    // ==============
    double tFase1 = difftime(tFimFase1, tInicio);
    double tFase2 = difftime(tFimFase2, tFimFase1);
    double tFase3 = difftime(tFimFase3, tFimFase2);
    double tFase4 = difftime(tFimFase4, tFimFase3);
    double tTotal = difftime(tFimFase4, tInicio);
    // ==============
    
    switch(cplex.getStatus()){
        case IloAlgorithm::Optimal:  status = "Optimal";   break;
        case IloAlgorithm::Feasible: status = "Feasible";  break;
        default:                     status = "No Solution";
    }

    cout << endl << endl;
    cout << "Status da FO: " << status << endl;

    if(cplex.getStatus() == IloAlgorithm::Optimal || cplex.getStatus() == IloAlgorithm::Feasible){ 
        objValue = cplex.getObjValue();
        // runTime = difftime(timer2, timer);

        cout << "\n\n\n\n\n\n\n";

        printf("\n================= CENÁRIO DE ENTRADA =================\n");
        printf("Paradas (incluindo escola 0): %d\n", quantidadeParadas);
        printf("Estudantes: %d\n", quantidadeAlunos);
        printf("Onibus disponiveis: %d\n", quantidadeOnibus);
        printf("Rotas possiveis por onibus (limite superior): %d\n", quantidadeRotas);
        printf("Capacidade maxima por onibus (Q): %d\n", Q);
        printf("Passos maximos por rota: %d\n", quantidadePassos);
        printf("Arestas no grafo de paradas: %d\n", quantidadeArestas);
        printf("Maior distancia aluno-parada no arquivo: %d\n", maxDistancia);

        cout << "=========================================================\n\n\n";
        cout << "=========================================================\n";
        
        cout << "\n================ MODELO ================\n";
        cout << "Status: " << status << "\n";
        // cout << "Tempo: " << runTime << " s\n";

        cout << "Nos explorados: " << (long) cplex.getNnodes() << "\n\n";

        cout << "Tempo Fase 1 (custo): " << tFase1 << " s\n";
        cout << "Tempo Fase 2 (paradas): " << tFase2 << " s\n";
        cout << "Tempo Fase 3 (caminhada W): " << tFase3 << " s\n";
        cout << "Tempo Fase 4 (balanceamento M): " << tFase4 << " s\n";
        cout << "Tempo total: " << tTotal << " s\n\n";

		cout << "\n";
        cout << "========================================================\n";
        cout << "                  SOLUCAO ENCONTRADA\n";
        cout << "========================================================\n";

        // ======================================================
        // 1] ROTAS DOS ONIBUS (com distancia, alunos e ocupacao)
        // ======================================================
        cout << "\n[1] ROTAS DOS ONIBUS\n";
        cout << "--------------------------------------------------------\n";

        for(int k = 0; k < quantidadeOnibus; k++){

            if(cplex.getValue(z[k]) < 0.5) continue;

            cout << "\n>> Onibus " << k << "\n";

            for(int r = 0; r < quantidadeRotas; r++){

                if(cplex.getValue(t[k][r]) < 0.5) continue;

                // reconstroi o caminho e calcula custo/paradas da rota
                vector<int> caminho = {0};
                double custoRota = 0;

                for(int st = 0; st < quantidadePassos; st++){
                    int de = -1, para = -1;

                    for(int i = 0; i < quantidadeParadas && de == -1; i++){
                        for(int j = 0; j < quantidadeParadas; j++){
                            double val = cplex.getValue(x[k][r][st][i][j]);
                            if(IloRound(val) >= 1){ de = i; para = j; break; }
                        }
                    }

                    if(de == -1) continue;      // passo sem arco detectado, segue tentando
                    if(de == 0 && para == 0) continue; // self-loop ocioso

                    custoRota += grafoParadas[de][para];
                    caminho.push_back(para);
                }

                // conta alunos e ocupacao dessa rota
                int alunosNaRota = 0;
                vector<string> passageiros;
                for(int e = 0; e < quantidadeAlunos; e++){
                    for(int p = 0; p < (int) alunosParadas[e].paradasPossiveis.size(); p++){
                        if(cplex.getValue(y[k][r][e][p]) > 0.5){
                            alunosNaRota++;
                            passageiros.push_back(
                                "Aluno " + to_string(alunosParadas[e].id) +
                                " (parada " + to_string(alunosParadas[e].paradasPossiveis[p].first) + ")");
                        }
                    }
                }

                // paradas visitadas por essa rota
                vector<int> paradasRota;
                for(int i = 0; i < quantidadeParadas; i++)
                    if(cplex.getValue(p[k][r][i]) > 0.5) paradasRota.push_back(i);

                cout << "   Rota " << r << ":\n";
                cout << "     Trajeto: ";
                for(size_t idx = 0; idx < caminho.size(); idx++){
                    cout << caminho[idx];
                    if(idx + 1 < caminho.size()) cout << " -> ";
                }
                cout << "\n";
                cout << "     Distancia percorrida: " << custoRota << "\n";
                cout << "     Paradas visitadas (" << paradasRota.size() << "): ";
                for(size_t idx = 0; idx < paradasRota.size(); idx++){
                    cout << paradasRota[idx];
                    if(idx + 1 < paradasRota.size()) cout << ", ";
                }
                cout << "\n";
                cout << "     Ocupacao: " << alunosNaRota << " / " << Q << " alunos"
                     << " (" << (100.0 * alunosNaRota / Q) << "% da capacidade)\n";
                cout << "     Passageiros:\n";
                for(auto &p : passageiros) cout << "       - " << p << "\n";
                cout << "\n";
            }
        }

        // ======================================================
        // 2] PARADAS - USADAS E NAO USADAS
        // ======================================================
        cout << "[2] PARADAS\n";
        cout << "--------------------------------------------------------\n";

        vector<int> paradasUsadas, paradasNaoUsadas;
        for(int i = 0; i < quantidadeParadas; i++){
            if(cplex.getValue(b[i]) > 0.5) paradasUsadas.push_back(i);
            else paradasNaoUsadas.push_back(i);
        }

        cout << "Usadas (" << paradasUsadas.size() << "/" << quantidadeParadas << "): ";
        for(size_t idx = 0; idx < paradasUsadas.size(); idx++){
            cout << paradasUsadas[idx];
            if(idx + 1 < paradasUsadas.size()) cout << ", ";
        }
        cout << "\n";

        cout << "Nao usadas (" << paradasNaoUsadas.size() << "): ";
        for(size_t idx = 0; idx < paradasNaoUsadas.size(); idx++){
            cout << paradasNaoUsadas[idx];
            if(idx + 1 < paradasNaoUsadas.size()) cout << ", ";
        }
        cout << "\n\n";

        // ======================================================
        // 3] ALOCACAO DOS ALUNOS
        // ======================================================
        cout << "[3] ALOCACAO DOS ALUNOS\n";
        cout << "--------------------------------------------------------\n";

        int naoAlocados = 0;
        double somaDist = 0, maiorDist = 0;

        for(int e = 0; e < quantidadeAlunos; e++){

            bool encontrou = false;

            for(int p = 0; p < (int) alunosParadas[e].paradasPossiveis.size(); p++){

                if(cplex.getValue(a[e][p]) > 0.5){

                    int parada = alunosParadas[e].paradasPossiveis[p].first;
                    int distancia = alunosParadas[e].paradasPossiveis[p].second;

                    somaDist += distancia;
                    maiorDist = max(maiorDist, (double) distancia);

                    cout << "  Aluno " << alunosParadas[e].id
                         << " -> Parada " << parada
                         << " (caminhada: " << distancia << ")";

                    for(int k = 0; k < quantidadeOnibus; k++){
                        for(int r = 0; r < quantidadeRotas; r++){
                            if(cplex.getValue(y[k][r][e][p]) > 0.5){
                                cout << " | Onibus " << k << ", Rota " << r;
                                break;
                            }
                        }
                    }

                    cout << "\n";
                    encontrou = true;
                    break;
                }
            }

            if(!encontrou){
                naoAlocados++;
                cout << "  Aluno " << alunosParadas[e].id << " -> NAO ALOCADO !!\n";
            }
        }

        cout << "\nResumo de caminhada: media = "
             << (quantidadeAlunos - naoAlocados > 0 ? somaDist / (quantidadeAlunos - naoAlocados) : 0)
             << " | maior individual = " << maiorDist
             << " | alunos nao alocados = " << naoAlocados << "\n\n";

        // ======================================================
        // 4] ONIBUS - RESUMO
        // ======================================================
        cout << "[4] ONIBUS\n";
        cout << "--------------------------------------------------------\n";

        for(int k = 0; k < quantidadeOnibus; k++){
            int rotasDoOnibus = 0;
            for(int r = 0; r < quantidadeRotas; r++)
                if(cplex.getValue(t[k][r]) > 0.5) rotasDoOnibus++;

            cout << "  Onibus " << k << ": "
                 << (cplex.getValue(z[k]) > 0.5 ? "UTILIZADO" : "nao utilizado")
                 << " | rotas atribuidas: " << rotasDoOnibus << "\n";
        }

        cout << "\n";
        cout << "========================================================\n";
        cout << "                   RESUMO GERAL\n";
        cout << "========================================================\n";
        cout << "Alunos atendidos:      " << (quantidadeAlunos - naoAlocados) << " / " << quantidadeAlunos << "\n";
        cout << "Paradas ativas:        " << paradasUsadas.size() << " / " << quantidadeParadas << "\n";
        cout << "Onibus utilizados:     ";
        {
            int cont = 0;
            for(int k = 0; k < quantidadeOnibus; k++) if(cplex.getValue(z[k]) > 0.5) cont++;
            cout << cont << " / " << quantidadeOnibus << "\n";
        }
        cout << "Custo total das rotas: " << melhorCusto << "\n";
        cout << "Maior caminhada (W):   " << melhorDistW << "\n";
        cout << "Maior rotas/onibus(M): " << cplex.getValue(M) << "\n";
        cout << "========================================================\n";
        

	}else{
		printf("No Solution!\n");
	}


	cplex.end();
	obj1.end();
	obj2.end();

	cout << "Memory usage before end:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
	env.end();
	} catch(IloException& e){
    cerr << "CPLEX Exception: " << e << endl;
    throw;
	}
}

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;

    dados.leitura(argc[1]);

    dados.cplex();

    return 0;
}