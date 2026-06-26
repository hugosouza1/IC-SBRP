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

    // (parada, distancia)
    vector<pair<int, int>> paradasPossiveis;
};

class infoSBRP{
	private:
	    // matriz de distâncias/custos entre paradas
		int quantidadeArestas;
	    vector<vector<int>> grafoParadas;

	    // estudantes
	    vector<estudante> alunosParadas;
        
        // pra impressao
        vector<vector<int>> next;

	    int quantidadeParadas;
	    int quantidadeAlunos;
	    int quantidadeOnibus;
	    int quantidadeRotas;

	    // capacidade do ônibus
	    int Q;

	    // maior distância permitida (W)
	    int maxDistancia;

	public:

	    void leitura(std::string arquivoEntrada);
        vector<int> caminhoReal(int origem, int destino);
	    void cplex();
};

vector<int> infoSBRP::caminhoReal(int origem, int destino) {
    if(next[origem][destino] == -1) return {}; 
    vector<int> caminho;
    caminho.push_back(origem);
    while(origem != destino) {
        origem = next[origem][destino];
        caminho.push_back(origem);
    }
    return caminho;
}

void infoSBRP::leitura(string arquivoEntrada){
    ifstream arq(arquivoEntrada);
    if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1);}

    arq >> quantidadeParadas >> quantidadeAlunos >> quantidadeOnibus >> Q;
    quantidadeRotas = quantidadeAlunos / 2;

    grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, INF));
    for(int i = 0; i < quantidadeParadas; i++){
        grafoParadas[i][i] = 0;
    }

    // arq >> quantidadeArestas;
    // for(int i = 0; i < quantidadeArestas; i++){
    //     int pontoA, pontoB, peso; 
    //     arq >> pontoA >> pontoB >> peso;
    //     grafoParadas[pontoA][pontoB] = min(grafoParadas[pontoA][pontoB], peso);
    //     grafoParadas[pontoB][pontoA] = min(grafoParadas[pontoB][pontoA], peso);
    // }

    // // Floyd-Warshall - virtualização de rotas  // tentar achar de novo sem
    // for(int k = 0; k < quantidadeParadas; k++){
    //     for(int i = 0; i < quantidadeParadas; i++){
    //         for(int j = 0; j < quantidadeParadas; j++){
    //             if(grafoParadas[i][k] != INF && grafoParadas[k][j] != INF) {
    //                 if(grafoParadas[i][k] + grafoParadas[k][j] < grafoParadas[i][j]){
    //                     grafoParadas[i][j] = grafoParadas[i][k] + grafoParadas[k][j];
    //                 }
    //             }
    //         }
    //     }
    // }

    arq >> quantidadeArestas;
    for(int i = 0; i < quantidadeArestas; i++){
        int pontoA, pontoB, peso; 
        arq >> pontoA >> pontoB >> peso;
        grafoParadas[pontoA][pontoB] = min(grafoParadas[pontoA][pontoB], peso);
        grafoParadas[pontoB][pontoA] = min(grafoParadas[pontoB][pontoA], peso);
    }

    next.assign(quantidadeParadas, vector<int>(quantidadeParadas, -1));
    for(int i = 0; i < quantidadeParadas; i++)
        for(int j = 0; j < quantidadeParadas; j++)
            if(i != j && grafoParadas[i][j] != INF)
                next[i][j] = j;

    // Floyd-Warshall 
    for(int k = 0; k < quantidadeParadas; k++){
        for(int i = 0; i < quantidadeParadas; i++){
            for(int j = 0; j < quantidadeParadas; j++){
                if(grafoParadas[i][k] != INF && grafoParadas[k][j] != INF) {
                    if(grafoParadas[i][k] + grafoParadas[k][j] < grafoParadas[i][j]){
                        grafoParadas[i][j] = grafoParadas[i][k] + grafoParadas[k][j];
                        next[i][j] = next[i][k]; 
                    }
                }
            }
        }
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
	
	//  tr^k
	IloArray<IloNumVarArray> t(env);
	
	for(int k = 0; k < quantidadeOnibus; k++){
		t.add(IloNumVarArray(env));
		
		for(int r = 0; r < quantidadeRotas; r++){
			t[k].add(IloIntVar(env, 0, 1));
			numberVar++;
		}
	}

	// x_kr^ij
    IloArray<IloArray<IloArray<IloNumVarArray>>> x(env);
    for(int k = 0; k < quantidadeOnibus; k++){
        x.add(IloArray<IloArray<IloNumVarArray>>(env));
        for(int r = 0; r < quantidadeRotas; r++){
            x[k].add(IloArray<IloNumVarArray>(env));
            for(int i = 0; i < quantidadeParadas; i++){
                x[k][r].add(IloNumVarArray(env));
                for(int j = 0; j < quantidadeParadas; j++){
                    
                    if(grafoParadas[i][j] == 0 || grafoParadas[i][j] == INF){
                        x[k][r][i].add(IloIntVar(env, 0, 0)); // Trava em 0
                    }
                    else{
                        x[k][r][i].add(IloIntVar(env, 0, 1)); // Variável binária normal
                    }
                    
                    numberVar++;
                }
            }
        }
    }

	// s_i^kr
    IloArray<IloArray<IloNumVarArray>> s(env);
    for(int k = 0; k < quantidadeOnibus; k++){
        s.add(IloArray<IloNumVarArray>(env));

        for(int r = 0; r < quantidadeRotas; r++){
            s[k].add(IloNumVarArray(env));

            for(int i = 0; i < quantidadeParadas; i++){
                s[k][r].add(IloIntVar(env, 0, 1));
                numberVar++;
            }
        }
    }

	// u_i^kr
    IloArray<IloArray<IloNumVarArray>> u(env);
	for(int k = 0; k < quantidadeOnibus; k++){
	    u.add(IloArray<IloNumVarArray>(env));
		
	    for(int r = 0; r < quantidadeRotas; r++){
	        u[k].add(IloNumVarArray(env));

	        for(int i = 0; i < quantidadeParadas; i++){
				if(i == 0)
				    u[k][r].add(IloIntVar(env, 0, 0));
				else
		            u[k][r].add( IloIntVar(env, 0, quantidadeParadas - 1));
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
    
	//Definicao do ambiente modelo ------------------------------------------
	IloModel model ( env );
	
	//FUNCAO OBJETIVO ---------------------------------------------
	IloExpr obj1(env); // custo
	IloExpr obj2(env); // qtd paradas
	// Restrição 1
    for(int k = 0; k < quantidadeOnibus; k++){
		for(int r = 0; r < quantidadeRotas; r++ ){
			for(int i = 0; i < quantidadeParadas; i++){
				for(int j = 0;  j < quantidadeParadas; j++){
					obj1 += (grafoParadas[i][j] * x[k][r][i][j]);
                }
            }    
        }
	}

    // Restrição 2
    for(int i = 0;  i < quantidadeParadas; i++){
		obj2 += (b[i]);
    }
	
	//RESTRICOES ---------------------------------------------	

    IloExpr soma(env); 

    // --- ALOCAÇÃO DE ESTUDANTES ÀS PARADAS ---
    
    // (5) Cada estudante deve ser alocado a exatamente uma parada valida
    for(int e = 0; e < quantidadeAlunos; e++) {
        soma.clear();
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            soma += a[e][p];
        }
        model.add(soma == 1);
        numberRes++;
    }

    // (6) Um estudante só pode ser alocado a uma parada se ela estiver ativa
    for(int e = 0; e < quantidadeAlunos; e++) {
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = alunosParadas[e].paradasPossiveis[p].first;
            model.add(a[e][p] <= b[i]);
            numberRes++;
        }
    }

    // (7) Define W como a maior distância de caminhada
    for(int e = 0; e < quantidadeAlunos; e++) {
        for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = alunosParadas[e].paradasPossiveis[p].first;
            double d_ei = alunosParadas[e].paradasPossiveis[p].second;
            model.add(d_ei * a[e][p] <= W);
            numberRes++;
        }
    }

    // ======== CONSERVAÇÃO DE FLUXO E ROTEAMENTO ========

    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            
            // (8) Conservação de fluxo (Entrada = Saída) para cada nó i
            for(int i = 0; i < quantidadeParadas; i++) {
                IloExpr somaEntrada(env);
                IloExpr somaSaida(env);
                for(int j = 0; j < quantidadeParadas; j++) {
                    somaEntrada += x[k][r][j][i];
                    somaSaida   += x[k][r][i][j];
                }
                model.add(somaEntrada == somaSaida);
                numberRes++;
                somaEntrada.end(); 
                somaSaida.end();
            }

            // (9) Toda rota ativa deve sair da escola (0)
            soma.clear();
            for(int j = 1; j < quantidadeParadas; j++) {
                soma += x[k][r][0][j];
            }
            model.add(soma == t[k][r]);
            numberRes++;

            // (10) Toda rota ativa deve retornar à escola (0)
            soma.clear();
            for(int i = 1; i < quantidadeParadas; i++) {
                soma += x[k][r][i][0];
            }
            model.add(soma == t[k][r]);
            numberRes++;

            // (11) Proíbe self-loops
            soma.clear();
            for(int i = 0; i < quantidadeParadas; i++) {
                soma += x[k][r][i][i];
            }
            model.add(soma == 0);
            numberRes++;
        }
    }

    // --- ELIMINAÇÃO DE SUBCICLOS (MTZ) ---

    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            for(int i = 1; i < quantidadeParadas; i++) {
                for(int j = 1; j < quantidadeParadas; j++) {
                    if(i != j) {
                        // (12) MTZ : Força u_i + 1 <= u_j caso x_ij = 1
                        model.add(u[k][r][i] - u[k][r][j] + (quantidadeParadas * x[k][r][i][j]) <= quantidadeParadas - 1);
                        numberRes++;
                    }
                }
                
                // (13) Se a parada não é visitada (s = 0), zera u_i
                model.add(u[k][r][i] <= (quantidadeParadas - 1) * s[k][r][i]);
                numberRes++;

                // (14) Se a parada é visitada (s = 1), u_i tem que ser pelo menos 1
                model.add(u[k][r][i] >= s[k][r][i]);
                numberRes++;
            }
        }
    }

    // ============= ATIVAÇÃO E ASSOCIAÇÃO DE PARADAS =============

    // (15) Vincula o fluxo de entrada da aresta à ativação de visita da parada
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            for(int i = 0; i < quantidadeParadas; i++) {
                soma.clear();
                for(int j = 0; j < quantidadeParadas; j++) {
                    soma += x[k][r][j][i];
                }
                model.add(soma == s[k][r][i]);
                numberRes++;
            }
        }
    }

    // (16) Se a parada b_i está ativa, pelo menos uma rota deve visitá-la
    for(int i = 0; i < quantidadeParadas; i++) {
        soma.clear();
        for(int k = 0; k < quantidadeOnibus; k++) {
            for(int r = 0; r < quantidadeRotas; r++) {
                soma += s[k][r][i];
            }
        }
        model.add(soma >= b[i]);
        numberRes++;
    }

    // (17) Impede que veículos visitem uma parada desativada 
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            for(int i = 0; i < quantidadeParadas; i++) {
                model.add(s[k][r][i] <= b[i]);
                numberRes++;
            }
        }
    }

    // ============= CAPACIDADE DOS VEÍCULOS =============

    // (18) Limita a quantidade de estudantes atendidos na rota à capacidade máxima Q 
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
                for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++) {
                    int i = alunosParadas[e].paradasPossiveis[p].first;

                    // (19) y <= a
                    model.add(y[k][r][e][p] <= a[e][p]);
                    
                    // (20) y <= s
                    model.add(y[k][r][e][p] <= s[k][r][i]);
                    
                    // (21) y >= a + s - 1
                    model.add(y[k][r][e][p] >= a[e][p] + s[k][r][i] - 1);
                    
                    numberRes += 3;
                }
            }
        }
    }

    // ============= RELAÇÃO ÔNIBUS-ROTA =============

    // (22) Ativa z_k se o ônibus k operar ao menos uma rota
    for(int k = 0; k < quantidadeOnibus; k++) {
        soma.clear();
        for(int r = 0; r < quantidadeRotas; r++) {
            soma += t[k][r];
        }
        model.add(soma >= z[k]);
        numberRes++;
    }

    // (23) Impede rotas em ônibus desativados
    for(int k = 0; k < quantidadeOnibus; k++) {
        for(int r = 0; r < quantidadeRotas; r++) {
            model.add(t[k][r] <= z[k]);
            numberRes++;
        }
    }

    // (24) Cada rota  r só pode pertencer a no máximo um veículo k
    for(int r = 0; r < quantidadeRotas; r++) {
        soma.clear();
        for(int k = 0; k < quantidadeOnibus; k++) {
            soma += t[k][r];
        }
        model.add(soma <= 1);
        numberRes++;
    }

    // ============= BALANCEAMENTO =============

    // (25) M
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
    time_t timer, timer2;
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

    time(&timer);

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
    model.add(obj1 <= melhorCusto + 1e-4); // Fixa o melhor custo com tolerância numérica

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

    // ================= FASE 4: Minimizar Max Rotas (M) =================
    IloObjective FO4 = IloMinimize(env, M);
    model.add(FO4);
    cplex.extract(model);
    cplex.solve();
    
    time(&timer2);
    
    switch(cplex.getStatus()){
        case IloAlgorithm::Optimal:  status = "Optimal";   break;
        case IloAlgorithm::Feasible: status = "Feasible";  break;
        default:                     status = "No Solution";
    }

    cout << endl << endl;
    cout << "Status da FO: " << status << endl;

    if(cplex.getStatus() == IloAlgorithm::Optimal || cplex.getStatus() == IloAlgorithm::Feasible){ 
        objValue = cplex.getObjValue();
        runTime = difftime(timer2, timer);
        
        cout << "\n================ SOLUCAO ================\n";
        cout << "Status: " << status << "\n";
        cout << "Tempo: " << runTime << " s\n";
        cout << "Custo Total: " << melhorCusto << "\n";
        cout << "Paradas Utilizadas: " << melhorQtdParadas << "\n";
        cout << "W (maior distancia de caminhada individual): " << melhorDistW << "\n"; 
        cout << "M (max rotas por onibus): " << cplex.getValue(M) << "\n\n";

		cout << "\n========================================================\n";
		cout << "SOLUCAO ENCONTRADA\n";
		cout << "========================================================\n";


		// ======================================================
		// 1 ROTAS
		// ======================================================

		cout << "\n[1] ROTAS DOS ONIBUS\n";

		for(int k = 0; k < quantidadeOnibus; k++){

		    if(cplex.getValue(z[k]) < 0.5)
		        continue;

		    cout << "\nOnibus " << k << endl;

		    for(int r = 0; r < quantidadeRotas; r++){

		        if(cplex.getValue(t[k][r]) < 0.5)
		            continue;

		        cout << "  Rota " << r << ": ";

                int atual = 0;
                bool primeiro = true;
                vector<int> visitados;
                visitados.push_back(0);

                while(true){
                    bool encontrou = false;
                    for(int j = 0; j < quantidadeParadas; j++){
                        if(cplex.getValue(x[k][r][atual][j]) > 0.5){

                            vector<int> trecho = caminhoReal(atual, j);
                            for(int idx = (primeiro ? 0 : 1); idx < trecho.size(); idx++){
                                if(!primeiro || idx > 0) cout << " -> ";
                                cout << trecho[idx];
                            }
                            primeiro = false;

                            atual = j;
                            encontrou = true;

                            if(atual == 0) break;

                            if(find(visitados.begin(), visitados.end(), atual) != visitados.end()){
                                cout << " [SUBCICLO]";
                                break;
                            }
                            visitados.push_back(atual);
                            break;
                        }
                    }
                    if(!encontrou || atual == 0) break;
                }
                cout << endl;
		    }
		}


		// ======================================================
		// 2 PARADAS UTILIZADAS
		// ======================================================

		cout << "\n[2] PARADAS UTILIZADAS\n";

		for(int i = 0; i < quantidadeParadas; i++){

		    if(cplex.getValue(b[i]) > 0.5){

		        cout << "Parada " << i << endl;
		    }
		}


		// ======================================================
		// 3 ALUNOS ALOCADOS
		// ======================================================

		cout << "\n[3] ALOCACAO DOS ALUNOS\n";

		for(int e = 0; e < quantidadeAlunos; e++){

		    bool encontrou = false;

		    for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){

		        if(cplex.getValue(a[e][p]) > 0.5){

		            int parada = alunosParadas[e].paradasPossiveis[p].first;

		            int distancia = alunosParadas[e].paradasPossiveis[p].second;

		            cout
		                << "Aluno "
		                << alunosParadas[e].id
		                << " -> Parada "
		                << parada
		                << " (dist="
		                << distancia
		                << ")";

		            encontrou = true;

		            // descobrir qual rota atende esse aluno
		            for(int k = 0; k < quantidadeOnibus; k++){

		                for(int r = 0; r < quantidadeRotas; r++){

		                    if(cplex.getValue(y[k][r][e][p]) > 0.5){

		                        cout
		                            << " | Onibus "
		                            << k
		                            << " | Rota "
		                            << r;

		                        break;
		                    }
		                }
		            }

		            cout << endl;
		        }
		    }

		    if(!encontrou){

		        cout
		            << "Aluno "
		            << alunosParadas[e].id
		            << " nao alocado"
		            << endl;
		    }
		}

        // ======================================================
        // 4 ALUNOS POR ROTA
        // ======================================================

        cout << "\n[4] ALUNOS POR ÔNIBUS-ROTA\n";

        for(int k = 0; k < quantidadeOnibus; k++){
            if(cplex.getValue(z[k]) < 0.5) continue;

            for(int r = 0; r < quantidadeRotas; r++){
                if(cplex.getValue(t[k][r]) < 0.5) continue;

                cout << "Onibus " << k << ", Rota " << r << ": alunos ";

                bool primeiro = true;
                for(int e = 0; e < quantidadeAlunos; e++){
                    for(int p = 0; p < (int)alunosParadas[e].paradasPossiveis.size(); p++){
                        if(cplex.getValue(y[k][r][e][p]) > 0.5){
                            if(!primeiro) cout << ", ";
                            cout << alunosParadas[e].id;
                            primeiro = false;
                            break; 
                        }
                    }
                }

                if(primeiro) cout << "(nenhum)";
                cout << endl;
            }
        }


		// ======================================================
		// 5 PARADAS VISITADAS POR ROTA
		// ======================================================

		cout << "\n[5] PARADAS VISITADAS\n";

		for(int k = 0; k < quantidadeOnibus; k++){

		    for(int r = 0; r < quantidadeRotas; r++){

		        if(cplex.getValue(t[k][r]) < 0.5)
		            continue;

		        cout
		            << "Onibus "
		            << k
		            << " Rota "
		            << r
		            << ": ";

		        bool primeira = true;

		        for(int i = 0; i < quantidadeParadas; i++){

		            if(cplex.getValue(s[k][r][i]) > 0.5){

		                if(!primeira)
		                    cout << ", ";

		                cout << i;

		                primeira = false;
		            }
		        }

		        cout << endl;
		    }
		}


		// ======================================================
		// 6 ÔNIBUS
		// ======================================================

		cout << "\n[6] Ônibus\n";

		for(int k = 0; k < quantidadeOnibus; k++){

		    cout
		        << "Onibus "
		        << k
		        << " utilizado = "
		        << cplex.getValue(z[k])
		        << endl;
		}

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

    // heuristica aqui

    dados.cplex();

    return 0;
}