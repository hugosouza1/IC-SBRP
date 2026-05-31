#include <iostream>
#include <fstream>
#include <vector>
#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>

using namespace std;
ILOSTLBEGIN //MACRO - "using namespace" for ILOCPEX

//CPLEX Parameters
#define CPLEX_TIME_LIM 3600 //3600 segundos
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
	    void cplex();
};

void infoSBRP::leitura(std::string arquivoEntrada){

    ifstream arq(arquivoEntrada);

    if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1);}

    /*
        FORMATO DO ARQUIVO

        quantidadeParadas
        quantidadeAlunos
        quantidadeOnibus
        capacidadeOnibus

        matriz NxN

        alunos...
    */

    arq >> quantidadeParadas;
    arq >> quantidadeAlunos;
    arq >> quantidadeOnibus;
    arq >> Q;

	// uma rota por aluno no pior caso e vai reduzinfo. Arrumar depois a inicialização
	quantidadeRotas = quantidadeAlunos / 2;

    grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, 0));

	arq >> quantidadeArestas;
	for(int i = 0; i < quantidadeArestas; i++){
		int pontoA, pontoB, peso; 
		arq >> pontoA >> pontoB >> peso;
		grafoParadas[pontoA][pontoB] = peso;
		grafoParadas[pontoB][pontoA] = peso;
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
					if(grafoParadas[i][j] == 0){
					    x[k][r][i].add(IloIntVar(env, 0, 0));
					}
					else{
					    x[k][r][i].add(IloIntVar(env, 0, 1));
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
	
	//Definicao do ambiente expressoes, para os somatorios ---------------------------------
	//Nota: Os somatorios podem ser reaproveitados usando o .clear(),
	//com excecao de quando existe mais de um somatorio em uma mesma restricao.
	
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
	IloExpr sum(env); /// Expression for Sum
	IloExpr sum2(env); /// Expression for Sum2

	
	// 3.5
	for(int e = 0; e < quantidadeAlunos; e++){
	    sum.clear();

	    for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	        sum += a[e][p];
	    }

	    model.add(sum == 1);
		numberRes++;
	}

	// Rest 3.6
	for(int e = 0; e < quantidadeAlunos; e++){

		// pair<parada, distancia>
	    for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	        int parada = alunosParadas[e].paradasPossiveis[p].first;
	        model.add( a[e][p] <= b[parada] );
			numberRes++;
	    }
	}

	
	// Restricao 3.7
	for(int e = 0; e < quantidadeAlunos; e++){

	    for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
			int distancia = alunosParadas[e].paradasPossiveis[p].second;
	        model.add( distancia * a[e][p] <= W );
			numberRes++;
	    }
	}
	
	// rest 3.8
    for(int k = 0; k < quantidadeOnibus; k++){
		for(int r = 0; r < quantidadeRotas; r++ ){
			
			for(int i = 0; i < quantidadeParadas; i++){
				sum.clear();
				sum2.clear();
				
				for(int j = 0;  j < infoSBRP::quantidadeParadas; j++){
					sum += (x[k][r][i][j]);
					sum2 += (x[k][r][j][i]);
                }
				
				model.add( sum == sum2 );
				numberRes++;

            }    
			
        }
	}

	// rest 3.9
    for(int k = 0; k < quantidadeOnibus; k++){
		for(int r = 0; r < quantidadeRotas; r++ ){

			sum.clear();

			for(int i = 0; i < quantidadeParadas; i++){
				sum += (x[k][r][0][i]);
            }

			model.add( sum == t[k][r]);
			numberRes++;
			
        }
	}

	// 3.10
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
	        for(int i = 0; i < quantidadeParadas; i++){

	            sum.clear();

	            for(int j = 0; j < quantidadeParadas; j++){
	                sum += x[k][r][i][j];
	            }

	            model.add(sum == s[k][r][i]);
	            numberRes++;
	        }
	    }
	}

	// 3.11
	for(int i = 0; i < quantidadeParadas; i++){
	    sum.clear();
		
	    for(int k = 0; k < quantidadeOnibus; k++){
			for(int r = 0; r < quantidadeRotas; r++){
				sum += s[k][r][i];
	        }
	    }
		
	    model.add(sum >= b[i]);
	}
	
	
	
	// 3.12
	sum.clear();
	for(int k = 0; k < quantidadeOnibus; k++){
		for(int r = 0; r < quantidadeRotas; r++){
			for(int i = 0; i < quantidadeParadas; i++){
				model.add( x[k][r][i][i] == 0 );
	            numberRes++;
	        }
	    }
	}
	
	// 3.13
    for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){

	        for(int i = 1; i < quantidadeParadas; i++){
	            for(int j = 1; j < quantidadeParadas; j++){
	                if(i == j)
	                    continue;

	                model.add( u[k][r][i] - u[k][r][j] + ((quantidadeParadas - 1) * x[k][r][i][j]) <= (quantidadeParadas - 1));
	                numberRes++;
	            }
	        }
	    }
	}

	// 3.14
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){

	        for(int i = 1; i < quantidadeParadas; i++){
	                model.add( u[k][r][i] <= (quantidadeParadas - 1) * s[k][r][i]);
	                numberRes++;
            }
        }
    }
	

	// 3.15
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){

	        for(int i = 1; i < quantidadeParadas; i++){
	                model.add( u[k][r][i] >= s[k][r][i]);
	                numberRes++;
            }
        }
    }



	// 3.16
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
			
	        sum.clear();
	        for(int e = 0; e < quantidadeAlunos; e++){
	            for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	                sum += y[k][r][e][p];
	            }
	        }

	        model.add(sum <= Q * t[k][r]);
			numberRes++;
	    }
	}

	// 3.17
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
	        for(int e = 0; e < quantidadeAlunos; e++){
	            for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
					model.add(y[k][r][e][p] <= a[e][p]);
					numberRes++;
	            }
	        }
	    }
	}

	// 3.18
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
	        for(int e = 0; e < quantidadeAlunos; e++){
	            for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	                int parada = alunosParadas[e].paradasPossiveis[p].first;
					model.add(y[k][r][e][p] <= s[k][r][parada]);
					numberRes++;
	            }
	        }
	    }
	}

	// 3.19
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
	        for(int e = 0; e < quantidadeAlunos; e++){
	            for(int p = 0; p < alunosParadas[e].paradasPossiveis.size(); p++){
	                int parada = alunosParadas[e].paradasPossiveis[p].first;
					model.add(y[k][r][e][p] >= s[k][r][parada] + a[e][p] - 1);
					numberRes++;
	            }
	        }
	    }
	}

	// 3.20
	for(int k = 0; k < quantidadeOnibus; k++){

		sum.clear();
	    for(int r = 0; r < quantidadeRotas; r++){
			sum += t[k][r];
		}

		model.add(sum >= z[k]);
		numberRes++;
	}
	
	// 3.21
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
			model.add(t[k][r] <= z[k]);
			numberRes++;
		}
	}
		
	// 3.22
	for(int r = 0; r < quantidadeRotas; r++){
		
		sum.clear();
		for(int k = 0; k < quantidadeOnibus; k++){
		sum += t[k][r];
		}
	
		model.add(sum <= 1);
		numberRes++;
}

	// 3.23
	for(int k = 0; k < quantidadeOnibus; k++){
	    for(int r = 0; r < quantidadeRotas; r++){
	        for(int i = 0; i < quantidadeParadas; i++){
	            for(int j = 0; j < quantidadeParadas; j++){
					model.add( x[k][r][i][j] <= t[k][r]);
					numberRes++;
	            }
	        }
	    }
	}
	
	// 3.24
	for(int k = 0; k < quantidadeOnibus; k++){

	    sum.clear();
	    for(int r = 0; r < quantidadeRotas; r++){
	        sum += t[k][r];
	    }
			
	    model.add(M >= sum);  // M >= total de rotas do ônibus k
	    numberRes++;
	}

	//------ EXECUCAO do MODELO ----------
	time_t timer, timer2;
	IloNum value, objValue;
	double runTime;
	string status;
	
	//Informacoes ---------------------------------------------	
	printf("--------Informacoes da Execucao:----------\n\n");
	printf("#Var: %d\n", numberVar);
	printf("#Restricoes: %d\n", numberRes);
	cout << "Memory usage after variable creation:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
	
	IloCplex cplex(model);
	cout << "Memory usage after cplex(Model):  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;

	//Setting CPLEX Parameters
	cplex.setParam(IloCplex::TiLim, CPLEX_TIME_LIM);
	//cplex.setParam(IloCplex::TreLim, CPLEX_COMPRESSED_TREE_MEM_LIM);
	//cplex.setParam(IloCplex::WorkMem, CPLEX_WORK_MEM_LIM);
	//cplex.setParam(IloCplex::VarSel, CPLEX_VARSEL_MODE);

	time(&timer);

	// Fase 1 - primeira função objetivo
	IloObjective FO1 = IloMinimize(env, obj1);
	model.add(FO1);
	cplex.solve();
	
	// Fase 2
	double melhorCusto = cplex.getObjValue();
	model.remove(FO1);
	model.add(obj1 <= melhorCusto);

	IloObjective FO2 = IloMinimize(env, obj2);
	model.add(FO2);
	cplex.extract(model);
	cplex.solve();

	// Fase 3
	double melhorQtdParadas = cplex.getObjValue();
	model.remove(FO2);
	model.add(obj2 <= melhorQtdParadas);

	IloObjective FO3 = IloMinimize(env, W);
	model.add(FO3);
	cplex.extract(model);	
	cplex.solve();	

	if(cplex.getStatus() == IloAlgorithm::Infeasible){
		cerr << "\n\n\n\n\n";
		return;
	}
	
	// Fase 4
	double melhorDistW = cplex.getObjValue();
	model.remove(FO3); 
	model.add(W <= melhorDistW);

	IloObjective FO4 = IloMinimize(env, M);
	model.add(FO4);
	cplex.extract(model);
	cplex.solve();
	
	time(&timer2);
	
	//cout << "Solution Status: " << cplex.getStatus() << endl;
	//Results
	bool sol = true;
	/*
	Possible Status:
	- Unknown	 
	- Feasible	 
	- Optimal	 
	- Infeasible	 
	- Unbounded	 
	- InfeasibleOrUnbounded	 
	- Error
	*/
	switch(cplex.getStatus()){
		case IloAlgorithm::Optimal: 
			status = "Optimal";
			break;
		case IloAlgorithm::Feasible: 
			status = "Feasible";
			break;
		default: 
			status = "No Solution";
			sol = false;
	}

	cout << endl << endl;
	cout << "Status da FO: " << status << endl;

	if(sol){ 

		// cplex.exportModel("modelo.lp");
		//Results
		//int Nbin, Nint, Ncols, Nrows, Nnodes, Nnodes64;
		objValue = cplex.getObjValue();
		runTime = difftime(timer2, timer);
		//Informacoes Adicionais
		//Nbin = cplex.getNbinVars();
		//Nint = cplex.getNintVars();
		//Ncols = cplex.getNcols();
		//Nrows = cplex.getNrows();
		//Nnodes = cplex.getNnodes();
		//Nnodes64 = cplex.getNnodes64();
		//float gap; gap = cplex.getMIPRelativeGap();
		
		cout << "\n================ SOLUCAO ================\n";

		cout << "Status: " << status << "\n";
		cout << "Tempo: " << runTime << " s\n";

		cout << "Custo Total: " << melhorCusto << "\n";
		cout << "Paradas Utilizadas: " << melhorQtdParadas << "\n";
		cout << "W (maior distancia de caminhada individual): " << objValue << "\n"; 
		cout << "M (max rotas por onibus): " << cplex.getValue(M) << "\n\n";


		cout << "Atribuicao de alunos:\n";

		for(int e = 0; e < quantidadeAlunos; e++){

		    for(int p = 0;
		        p < alunosParadas[e].paradasPossiveis.size();
		        p++){

		        if(cplex.getValue(a[e][p]) > 0.5){

		            int parada =
		                alunosParadas[e]
		                .paradasPossiveis[p]
		                .first;

		            int distancia =
		                alunosParadas[e]
		                .paradasPossiveis[p]
		                .second;

		            cout
		                << "Aluno "
		                << alunosParadas[e].id
		                << " -> Parada "
		                << parada
		                << " (dist="
		                << distancia
		                << ")\n";
		        }
		    }
		}

		cout << "\n";

		for(int k = 0; k < quantidadeOnibus; k++){
		    int totalRotas = 0;
		    for(int r = 0; r < quantidadeRotas; r++){
		        if(cplex.getValue(t[k][r]) > 0.5) totalRotas++;
		    }
		    cout << "Onibus " << k << ": " << totalRotas << " rotas\n";
		}

		cout << "\nRotas: \n";
		for(int k = 0; k < quantidadeOnibus; k++){
		    for(int r = 0; r < quantidadeRotas; r++){
		        for(int i = 0; i < quantidadeParadas; i++){
		            for(int j = 0; j < quantidadeParadas; j++){

		                double val =
		                    cplex.getValue(x[k][r][i][j]);

				             if(val > 0.5){
								 cout
								 << "Onibus " << k << ", Rota "<< r << ": ponto "
								 << i << " -> ponto "
								 << j <<
								 endl;
								 
								}
		                
		            }
		        }
		    }
		}

		cout << "\nAlunos por parada:\n";

		for(int i = 0; i < quantidadeParadas; i++){

		    cout << "Parada " << i << ": ";

		    bool primeiro = true;

		    for(int e = 0; e < quantidadeAlunos; e++){

		        for(int p = 0;
		            p < alunosParadas[e].paradasPossiveis.size();
		            p++){

		            int parada =
		                alunosParadas[e]
		                .paradasPossiveis[p]
		                .first;

		            if(parada != i)
		                continue;

		            if(cplex.getValue(a[e][p]) > 0.5){

		                if(!primeiro)
		                    cout << ", ";

		                cout << alunosParadas[e].id;

		                primeiro = false;
		            }
		        }
		    }

		    cout << "\n";
		}

	}else{
		printf("No Solution!\n");
	}

	//Free Memory
	cplex.end();
	sum.end();
	sum2.end();
	obj1.end();
	obj2.end();

	cout << "Memory usage before end:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
	env.end();
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