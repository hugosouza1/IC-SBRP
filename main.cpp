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
    vector<vector<int>> grafoParadas;
    vector<estudante> alunosParadas;

    int quantidadeParadas;
    int quantidadeAlunos;
    int quantidadeOnibus;

    vector<vector<int>> rotas;


public:
    void cplex();
    void leitura(string entrada);
};

void infoSBRP::leitura(string arquivoEntrada) {

    ifstream arq(arquivoEntrada);

    if (!arq.is_open()) {
        cerr << "Erro ao abrir arquivo\n";
        exit(1);
    }

    arq >> quantidadeParadas;
    arq >> quantidadeAlunos;
    arq >> quantidadeOnibus;

    grafoParadas.resize(quantidadeParadas,vector<int>(quantidadeParadas));

    // leitura da matriz de paradas
    for (int i = 0; i < quantidadeParadas; i++) {
        for (int j = 0; j < quantidadeParadas; j++) {
            arq >> grafoParadas[i][j];
        }
    }

    // leitura dos alunos
    for (int i = 0; i < quantidadeAlunos; i++) {
        
        // ex:
        // 0 2 = aluno 0 com duas paradas no alcance 
        // 1 10 = para da 1 com distancia 10
        // 3 15 
        estudante aluno;

        int quantidadeParadasPossiveis;

        arq >> aluno.id;
        arq >> quantidadeParadasPossiveis;

        for (int j = 0; j < quantidadeParadasPossiveis; j++) {

            int parada;
            int distancia;

            arq >> parada >> distancia;

            aluno.paradasPossiveis.push_back({
                parada,
                distancia
            });
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

	//VARIAVEIS DE DECISAO (x_i) binaria
	IloNumVarArray b(env);
	for( i = 0; i < quantidadeParadas; i++ ){
        b.add(IloIntVar(env, 0, 1));
		numberVar++;
	}

    IloNumVarArray t(env);
	for( i = 0; i < rotas.size(); i++ ){
		t.add(IloIntVar(env, 0, 1));
		numberVar++;
	}


	//AJUDA
	//Definicao - Variaveis de Decisao 2 dimensoes (x_ij) binarias
	IloArray<IloNumVarArray> a(env);
	for(int e = 0; e < quantidadeAlunos; e++ ){
        a.add(IloNumVarArray(env));

		for(int i = 0; i < quantidadeParadas; i++ ){
            a[e].add(IloIntVar(env, 0, 1));
			numberVar++;
        }
    }

    IloArray<IloArray<IloArray<IloNumVarArray>>> x(env);

    for(int k = 0; k < quantidadeOnibus; k++){
        x.add(IloArray<IloArray<IloNumVarArray>>(env));

        for(int r = 0; r < quantidadeRotas; r++){
            x[k].add(IloArray<IloNumVarArray>(env));

            for(int i = 0; i < quantidadeParadas; i++){
                x[k][r].add(IloNumVarArray(env));

                for(int j = 0; j < quantidadeParadas; j++){
                    x[k][r][i].add(IloIntVar(env, 0, 1));
                    numberVar++;
                }
            }
        }
    }

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
    }

	//Definicao do ambiente modelo ------------------------------------------
	IloModel model ( env );
	
	//Definicao do ambiente expressoes, para os somatorios ---------------------------------
	//Nota: Os somatorios podem ser reaproveitados usando o .clear(),
	//com excecao de quando existe mais de um somatorio em uma mesma restricao.
	IloExpr sum(env); /// Expression for Sum
	IloExpr sum2(env); /// Expression for Sum2

	//FUNCAO OBJETIVO ---------------------------------------------
	sum.clear();
    for(int k = 0; k < quantidadeOnibus; k++){
        for(int r = 0; r < rotas.size(); r++ ){
            for(int i = 0; i < quantidadeParadas; i++){
                for(int j = 0; < j < quantidadeParadas; j++){
                    sum += (grafoParadas[i][j] * x[k][r][i][j]);
                }
            }    
        }
	}
	model.add(IloMinimize(env, sum)); //Minimizacao
    
    // sum2.clear();
    for(int i = 0; < i < quantidadeParadas; i++){
        sum += (b[i]);
    }
    model.add(IloMinimize(env, sum2)); //Minimizacao
    
	//RESTRICOES ---------------------------------------------	
	 
	//R1 - Respeito da capacidade de Mochila
	for(int e = 0; e < quantidadeAlunos; e++){
        sum.clear();

        for(int i = 0; i < quantidadeParadas; i++){
            sum += a[e][i];
        }
        model.add(sum == 1);

        numberRes++;
    }		





	//AJUDA - Restricoes
	/*//
	Vou exemplificar uma situação em que a restrição contém somatórios independentes
	e contém um (Para Todo na direita)
	Nesse caso, o índice do (Para Todo) fica em um laço externo à restrição.
	Gerando assim, várias restrições para tal.
	
	Exemplo: 
	Restrição de Oferta (2) do PFCM:
	S (maiusculo) é um conjunto de origens, cada uma com uma qntd. de oferta Q (maiusculo).
	Supondo que temos os nós em S = {0 1 3} e 
	Q[0] = 10, Q[1] = 10 e Q[3] = 20 (ofertas individuais)

	//- A restrição é escrita assim para o ILOG CPLEX, basta fazer o teste de existência da aresta:
	//- Ou seja, se existe a aresta, então a variável entra no respectivo somatório.

	for(i=0; i<S.size(); i++){ // For que representa o (Para Todo).
		sum.clear(); //Somatório 1
		for( j = 0; j < N; j++ ){
			if(existe aresta A[S[i]][j] ) //S[i] porque o índice real do vértice está dentro do conjunto S.
				sum += x[ S[i] ][ j ];
		}

		sum2.clear(); //Somatório 2
		for( k = 0; k < N; k++ ){
			if(existe aresta A[k][S[i]] ) //S[i] porque o índice real do vértice está dentro do conjunto S.
				sum2 += x[ k ][ S[i] ];
		}
		model.add(sum - sum2 <= Q[ S[i] ]); 
		numberRes++;
	}//Fim do for que representa o (Para Todo).

	Note que por esse exemplo, 3 restrições são adicionadas ao modelo
	por causa do tamanho do conjunto S, uma para cada vértice origem.
	*/


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
	cplex.solve();//COMANDO DE EXECUCAO
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
		
		cout << "Variaveis de decisao: " << endl;
		for( i = 0; i < N; i++ ){
			value = IloRound(cplex.getValue(x[i]));
			printf("x[%d]: %.0lf\n", i, value);
		}
		printf("\n");
		
		cout << "Funcao Objetivo Valor = " << objValue << endl;
		printf("..(%.6lf seconds).\n\n", runTime);

	}else{
		printf("No Solution!\n");
	}

	//Free Memory
	cplex.end();
	sum.end();
	sum2.end();

	cout << "Memory usage before end:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
	env.end();
}


int main(int argv, char *argc[]){
    if(argv < 1){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados();

    dados.leitura(argc[1]);

    // heuristica aqui


	printf("Verificacao da leitura dos dados:\n");
	printf("Num. Itens: %d\n", N);
	printf("Capacidade da mochila: %d\n", B);
	printf("Itens - id: peso valor\n");
    for(i=0; i<N; i++)
        printf("%d: %d %d\n", itens[i].id, itens[i].w, itens[i].c);
	printf("\n");

    cplex(dados);

    return 0;
}