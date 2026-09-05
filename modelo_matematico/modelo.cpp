#include "modelo.hpp"


void ModeloMatematico::verificarWarmStart(
    Individuo* solucao,
    const vector<bool>& rotaUsada,
    const vector<int>& rotaParaOnibus,
    IloArray<IloNumVarArray>& a,
    IloNumVarArray& b,
    IloNumVarArray& z,
    IloArray<IloNumVarArray>& t,
    IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>>& x,
    IloArray<IloArray<IloNumVarArray>>& p,
    IloArray<IloArray<IloArray<IloNumVarArray>>>& y,
    IloNumVar& W,
    IloNumVar& M,
    IloNumVarArray& vars,
    IloNumArray& vals)
{

    // valor que cada variavel recebeu no warm start (0 se nao foi listada)
    unordered_map<IloInt, double> valorFixado;
    valorFixado.reserve(vars.getSize() * 2);
    for(IloInt idx = 0; idx < vars.getSize(); idx++)
        valorFixado[vars[idx].getId()] = vals[idx];

    auto V = [&](const IloNumVar& v) -> double {
        auto it = valorFixado.find(v.getId());
        return (it != valorFixado.end()) ? it->second : 0.0;
    };

    int totalChecagens = 0, totalViolacoes = 0;
    vector<string> violacoes;

    // imprimir=false ainda checa e conta, so nao imprime se estiver OK
    // (violacoes sempre aparecem, mesmo com imprimir=false)
    auto checar = [&](const string& desc, double lhs, const string& op, double rhs, bool imprimir) {
        totalChecagens++;
        bool ok;
        if(op == "<=")      ok = lhs <= rhs + 1e-6;
        else if(op == ">=") ok = lhs >= rhs - 1e-6;
        else                ok = fabs(lhs - rhs) <= 1e-6;

        if(!ok) {
            totalViolacoes++;
            violacoes.push_back(desc + "  =>  " + to_string(lhs) + " " + op + " " + to_string(rhs));
        }
        
        string aux = to_string((int)rhs);
        // cout << "-+=" << aux << "-+=" ;
        // int tu = 
        string espacoBonito(max(1, 4 - (int)aux.size()), ' ');
        
        if(imprimir || !ok) {
            cout << "  " << left << setw(48) << desc
                 << lhs << " " << op << " " << rhs << espacoBonito
                 << (ok ? "[OK]" : "[!!! VIOLADA !!!]") << "\n";
        }
    };

    cout << "\n========================================================\n";
    cout << "         VERIFICACAO DO WARM START (pre-CPLEX)\n";
    cout << "========================================================\n";

    // [1] soma_p a[e][p] == 1
    cout << "\n[1] Alocacao de alunos -- soma_p a[e][p] == 1\n";
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        double soma = 0;
        for(int pos = 0; pos < (int) dados.alunosParadas[e].paradasPossiveis.size(); pos++)
            soma += V(a[e][pos]);
        checar("aluno " + to_string(e), soma, "==", 1.0, true);
    }

    // [2] a[e][p] <= b[i]  (agregado)
    cout << "\n[2] a[e][p] <= b[i]  (";
    {
        int n = 0;
        for(int e = 0; e < dados.quantidadeAlunos; e++)
            for(int pos = 0; pos < (int) dados.alunosParadas[e].paradasPossiveis.size(); pos++) {
                int i = dados.alunosParadas[e].paradasPossiveis[pos].first;
                checar("aluno " + to_string(e) + ", parada " + to_string(i),
                       V(a[e][pos]), "<=", V(b[i]), false);
                n++;
            }
        cout << n << " checagens)\n";
    }

    // [3] soma_ij x[k][r][st][i][j] <= 1
    cout << "\n[3] Um arco por passo -- soma x[k][r][st][*][*] <= 1  (so passos com arco ativo)\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        for(int st = 0; st < dados.quantidadePassos; st++) {
            double soma = 0;
            for(int i = 0; i < dados.quantidadeParadas; i++)
                for(int j = 0; j < dados.quantidadeParadas; j++)
                    soma += V(x[k][r][st][i][j]);
            if(soma > 1e-6)
                checar("onibus " + to_string(k) + ", rota " + to_string(r) + ", passo " + to_string(st),
                       soma, "<=", 1.0, true);
        }
    }

    // [4] soma_j x[k][r][0][0][j] == t[k][r]
    cout << "\n[4] Saida da escola (passo 0) -- soma_j x[k][r][0][0][j] == t[k][r]\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        double soma = 0;
        for(int j = 1; j < dados.quantidadeParadas; j++)
            soma += V(x[k][r][0][0][j]);
        checar("onibus " + to_string(k) + ", rota " + to_string(r), soma, "==", V(t[k][r]), true);
    }

    // [5] x[k][r][0][0][0] == 1 - t[k][r]
    cout << "\n[5] Self-loop inicial -- x[k][r][0][0][0] == 1 - t[k][r]\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        checar("onibus " + to_string(k) + ", rota " + to_string(r),
               V(x[k][r][0][0][0]), "==", 1.0 - V(t[k][r]), true);
    }

    // [6] retorno a escola
    cout << "\n[6] Retorno a escola -- soma x[k][r][*][i][0] (i!=0) == t[k][r]\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        double soma = 0;
        for(int st = 0; st < dados.quantidadePassos; st++)
            for(int i = 1; i < dados.quantidadeParadas; i++)
                soma += V(x[k][r][st][i][0]);
        checar("onibus " + to_string(k) + ", rota " + to_string(r), soma, "==", V(t[k][r]), true);
    }

    // [7] p[k][r][i] coerente com entradas
    cout << "\n[7] Parada ativada -- p[k][r][i] coerente com arcos que chegam em i\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        for(int i = 0; i < dados.quantidadeParadas; i++) {
            double entradas = 0;
            for(int st = 0; st < dados.quantidadePassos; st++)
                for(int j = 0; j < dados.quantidadeParadas; j++)
                    entradas += V(x[k][r][st][j][i]);
            if(entradas < 1e-6 && V(p[k][r][i]) < 1e-6) continue;
            string desc = "onibus " + to_string(k) + ", rota " + to_string(r) + ", parada " + to_string(i);
            checar(desc + " (p<=entradas)", V(p[k][r][i]), "<=", entradas, true);
            checar(desc + " (entradas<=passos*p)", entradas, "<=",
                   dados.quantidadePassos * V(p[k][r][i]), false);
        }
    }

    // [8] CAPACIDADE -- o mais importante dado o historico de bugs
    cout << "\n[8] Capacidade -- soma y[k][r][*][*] <= Q * t[k][r]\n";
    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        double soma = 0;
        for(int e = 0; e < dados.quantidadeAlunos; e++)
            for(int pos = 0; pos < (int) dados.alunosParadas[e].paradasPossiveis.size(); pos++)
                soma += V(y[k][r][e][pos]);
        checar("onibus " + to_string(k) + ", rota " + to_string(r),
               soma, "<=", dados.Q * V(t[k][r]), true);
    }

    // [9] linearizacao de y (a que corrigimos)
    cout << "\n[9] Linearizacao de y -- soma_(k,r) y[k][r][e][pp] == a[e][pp]  (so paradas escolhidas)\n";
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        for(int pos = 0; pos < (int) dados.alunosParadas[e].paradasPossiveis.size(); pos++) {
            double soma = 0;
            for(int k = 0; k < dados.quantidadeOnibus; k++)
                for(int r = 0; r < dados.quantidadeRotas; r++)
                    soma += V(y[k][r][e][pos]);
            int i = dados.alunosParadas[e].paradasPossiveis[pos].first;
            checar("aluno " + to_string(e) + ", parada " + to_string(i),
                   soma, "==", V(a[e][pos]), V(a[e][pos]) > 0.5);
        }
    }

    // [10] y<=a e y<=p (agregado)
    cout << "\n[10] y<=a  e  y<=p  (";
    {
        int n = 0;
        for(int k = 0; k < dados.quantidadeOnibus; k++)
            for(int r = 0; r < dados.quantidadeRotas; r++)
                for(int e = 0; e < dados.quantidadeAlunos; e++)
                    for(int pos = 0; pos < (int) dados.alunosParadas[e].paradasPossiveis.size(); pos++) {
                        int i = dados.alunosParadas[e].paradasPossiveis[pos].first;
                        string desc = "onibus " + to_string(k) + ", rota " + to_string(r) +
                                      ", aluno " + to_string(e) + ", parada " + to_string(i);
                        checar(desc + " (y<=a)", V(y[k][r][e][pos]), "<=", V(a[e][pos]), false);
                        checar(desc + " (y<=p)", V(y[k][r][e][pos]), "<=", V(p[k][r][i]), false);
                        n += 2;
                    }
        cout << n << " checagens)\n";
    }

    // [11] b[i] exige >=1 rota visitando
    cout << "\n[11] Parada ativa exige >=1 rota -- soma p[*][*][i] >= b[i]\n";
    for(int i = 0; i < dados.quantidadeParadas; i++) {
        if(V(b[i]) < 1e-6) continue;
        double soma = 0;
        for(int k = 0; k < dados.quantidadeOnibus; k++)
            for(int r = 0; r < dados.quantidadeRotas; r++)
                soma += V(p[k][r][i]);
        checar("parada " + to_string(i), soma, ">=", V(b[i]), true);
    }

    // [12] z[k]
    cout << "\n[12] Onibus usado -- soma_r t[k][r] >= z[k]\n";
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        double soma = 0;
        for(int r = 0; r < dados.quantidadeRotas; r++)
            soma += V(t[k][r]);
        if(soma < 1e-6 && V(z[k]) < 1e-6) continue;
        checar("onibus " + to_string(k), soma, ">=", V(z[k]), true);
    }

    // [13] ordem de rotas
    cout << "\n[13] Ordem de rotas -- soma_k t[k][r] <= soma_k t[k][r-1]\n";
    for(int r = 1; r < dados.quantidadeRotas; r++) {
        double somaAtual = 0, somaAnterior = 0;
        for(int k = 0; k < dados.quantidadeOnibus; k++) {
            somaAtual    += V(t[k][r]);
            somaAnterior += V(t[k][r - 1]);
        }
        if(somaAtual < 1e-6 && somaAnterior < 1e-6) continue;
        checar("rota " + to_string(r) + " vs rota " + to_string(r - 1), somaAtual, "<=", somaAnterior, true);
    }

    // [14] ordem de onibus
    cout << "\n[14] Ordem de onibus -- z[k] <= z[k-1]\n";
    for(int k = 1; k < dados.quantidadeOnibus; k++) {
        if(V(z[k]) < 1e-6 && V(z[k - 1]) < 1e-6) continue;
        checar("onibus " + to_string(k) + " vs onibus " + to_string(k - 1), V(z[k]), "<=", V(z[k - 1]), true);
    }

    // [15] M
    cout << "\n[15] Balanceamento -- M >= soma_r t[k][r]\n";
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        double soma = 0;
        for(int r = 0; r < dados.quantidadeRotas; r++)
            soma += V(t[k][r]);
        if(soma < 1e-6) continue;
        checar("onibus " + to_string(k), V(M), ">=", soma, true);
    }

    cout << "\n========================================================\n";
    cout << "RESUMO: " << totalChecagens << " checagens, " << totalViolacoes << " violacao(oes)\n";
    if(!violacoes.empty()) {
        cout << "--------------------------------------------------------\n";
        for(auto& v : violacoes) cout << "  * " << v << "\n";
    }
    cout << "========================================================\n\n";
}
 



bool ModeloMatematico::montarWarmStart(
    Individuo* solucao,
    IloEnv& env,
    IloArray<IloNumVarArray>& a,
    IloNumVarArray& b,
    IloNumVarArray& z,
    IloArray<IloNumVarArray>& t,
    IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>>& x,
    IloArray<IloArray<IloNumVarArray>>& p,
    IloArray<IloArray<IloArray<IloNumVarArray>>>& y,
    IloNumVar& W,
    IloNumVar& M,
    IloNumVarArray& vars,
    IloNumArray& vals) {


    // for(int i = 0; i < solucao->rotasFeitas.size(); ++i){
    //     cout << "rotas vindas:\n";
    //     for(int j = 0; j < solucao->rotasFeitas[i].size() ; ++j){
    //         cout << solucao->rotasFeitas[i][j] << " - ";
    //     }
    //     cout << "\n";
    // }
    // cout << "\n";
    // cout << "\n";


    // ============================================================
    // 0. TAMANHOS BASICOS
    // ============================================================
    if((int)solucao->atrAlunoParada.size() != dados.quantidadeAlunos) {
        cerr << "[warmstart] atrAlunoParada tem tamanho " << solucao->atrAlunoParada.size() << ", esperado " << dados.quantidadeAlunos << "\n"; return false; }

    if((int)solucao->atrAlunoRota.size() != dados.quantidadeAlunos) {
        cerr << "[warmstart] atrAlunoRota tem tamanho " << solucao->atrAlunoRota.size() << ", esperado " << dados.quantidadeAlunos << "\n"; return false; }

    // ============================================================
    // 1. ROTAS EM USO + MAPEAMENTO ROTA -> ONIBUS 
    // ============================================================
    vector<bool> rotaUsada(dados.quantidadeRotas, false);
    for(int r = 0; r < dados.quantidadeRotas; r++)
        rotaUsada[r] = r < (int)solucao->rotasFeitas.size() && !solucao->rotasFeitas[r].empty();

    vector<int> rotaParaOnibus(dados.quantidadeRotas, -1);
    vector<int> rotasPorOnibus(dados.quantidadeOnibus, 0);
    {
        int usadas = 0;
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            if(!rotaUsada[r]) continue;
            int k = usadas % dados.quantidadeOnibus;
            rotaParaOnibus[r] = k;
            rotasPorOnibus[k]++;
            usadas++;
        }
    }

    // ============================================================
    // 2. VALIDACAO DE CADA ROTA: escola, tamanho, arestas reais
    //    e sobreposicao de paradas entre rotas distintas
    // ============================================================
    vector<vector<bool>> paradaVisitadaPorRota(dados.quantidadeRotas, vector<bool>(dados.quantidadeParadas, false));

    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        const vector<int>& rota = solucao->rotasFeitas[r];

        if(rota.front() != 0 || rota.back() != 0) { cerr << "[warmstart] rota " << r << " nao comeca/termina na escola (0).\n"; return false;}

        if((int)rota.size() - 1 > dados.quantidadePassos) { cerr << "[warmstart] rota " << r << " precisa de " << rota.size() - 1 << " passos, mas o modelo so tem " << dados.quantidadePassos << ".\n"; return false; }

        for(size_t st = 0; st + 1 < rota.size(); st++) {
            int i = rota[st], j = rota[st + 1];
            if(i != j && dados.grafoParadas[i][j] == 0) {
                cerr << "[warmstart] rota " << r << ", passo " << st << ": arco " << i
                     << " -> " << j << " nao existe no grafo "
                     << "(grafoParadas[" << i << "][" << j << "]=" << dados.grafoParadas[i][j]
                     << ", grafoParadas[" << j << "][" << i << "]=" << dados.grafoParadas[j][i] << ").\n";
                return false;
            }
            paradaVisitadaPorRota[r][i] = true;
        }
        paradaVisitadaPorRota[r][0] = true; // escola
    }
    

    // ============================================================
    // 3. ALOCACAO ALUNO -> PARADA (a) e PARADA ATIVA (b)
    // ============================================================
    vector<bool> paradaAtiva(dados.quantidadeParadas, false);

    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        int paradaEscolhida = solucao->atrAlunoParada[e];
        const auto& poss = dados.alunosParadas[e].paradasPossiveis;

        bool encontrada = false;
        for(int pos = 0; pos < (int)poss.size(); pos++) {
            bool eh = (poss[pos].first == paradaEscolhida);
            vars.add(a[e][pos]);
            vals.add(eh ? 1.0 : 0.0);
            if(eh) encontrada = true;
        }
        if(!encontrada) {
            cerr << "[warmstart] aluno " << e << " recebeu parada " << paradaEscolhida << ", que nao esta entre as possiveis dele.\n";
            return false;
        }
        paradaAtiva[paradaEscolhida] = true;
    }

    for(int i = 0; i < dados.quantidadeParadas; i++) {
        vars.add(b[i]);
        vals.add(paradaAtiva[i] ? 1.0 : 0.0);
    }

    // ============================================================
    // 4. ONIBUS -> ROTA (t), ARCOS (x) E PARADAS (p)
    // ============================================================
    for(int r = 0; r < dados.quantidadeRotas; r++)
        for(int k = 0; k < dados.quantidadeOnibus; k++) {
            vars.add(t[k][r]);
            vals.add((rotaUsada[r] && rotaParaOnibus[r] == k) ? 1.0 : 0.0);
        }

    for(int r = 0; r < dados.quantidadeRotas; r++) {
        if(!rotaUsada[r]) continue;
        int k = rotaParaOnibus[r];
        const vector<int>& rota = solucao->rotasFeitas[r];

        for(size_t st = 0; st + 1 < rota.size(); st++) {
            vars.add(x[k][r][st][rota[st]][rota[st + 1]]);
            vals.add(1.0);
        }
        for(int st = (int)rota.size() - 1; st < dados.quantidadePassos; st++) {
            vars.add(x[k][r][st][0][0]);
            vals.add(1.0);
        }
        for(int i = 0; i < dados.quantidadeParadas; i++) {
            vars.add(p[k][r][i]);
            vals.add(paradaVisitadaPorRota[r][i] ? 1.0 : 0.0);
        }
    }
    
    // ============================================================
    // 5. z[k] e M
    // ============================================================
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        vars.add(z[k]);
        vals.add(rotasPorOnibus[k] > 0 ? 1.0 : 0.0);
    }

    vars.add(M);
    vals.add((double) *max_element(rotasPorOnibus.begin(), rotasPorOnibus.end()));

    // ============================================================
    // 6. W
    // ============================================================
    double maiorW = 0;
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        int paradaEscolhida = solucao->atrAlunoParada[e];
        for(const auto& par : dados.alunosParadas[e].paradasPossiveis)
            if(par.first == paradaEscolhida)
                maiorW = max(maiorW, (double) par.second);
    }

    vars.add(W);
    vals.add(maiorW);

    // ============================================================
    // 7. y[k][r][e][pos] -- so na rota realmente atribuida ao aluno
    // ============================================================

    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            for(int e = 0; e < dados.quantidadeAlunos; e++) {
                int rotaAluno = solucao->atrAlunoRota[e];
                int paradaAluno = solucao->atrAlunoParada[e];

                for(int pp = 0; pp < dados.alunosParadas[e].paradasPossiveis.size(); pp++) {

                    int parada = dados.alunosParadas[e].paradasPossiveis[pp].first;

                    bool valor = (r == rotaAluno && k == rotaParaOnibus[rotaAluno] && parada == paradaAluno);

                    vars.add(y[k][r][e][pp]);
                    vals.add(valor ? 1.0 : 0.0);
                }
            }
        }
    }

    // ============================================================
    // 8. checagem extra de capacidade antes de mandar pro CPLEX
    // ============================================================

    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        
        int r = solucao->atrAlunoRota[e];
        if(r < 0 || r >= dados.quantidadeRotas || !rotaUsada[r]) {
            cerr << "[warmstart] aluno " << e << " atribuido a rota " << r << " que nao esta em uso.\n";
            return false;
        }
    }
    
    verificarWarmStart(solucao, rotaUsada, rotaParaOnibus, a, b, z, t, x, p, y, W, M, vars, vals);

    return true;
}



void ModeloMatematico::cplexSolver(Individuo* solucao){
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
	for(int i = 0; i < dados.quantidadeParadas; i++ ){
        b.add(IloIntVar(env, 0, 1));
		numberVar++;
	}

	// z
	IloNumVarArray z(env);
	for(int i = 0; i < dados.quantidadeOnibus; i++ ){
        z.add(IloIntVar(env, 0, 1));
		numberVar++;
	}

	
	
	// =========== Variaveis de Decisao 2 dimensoes (x_ij) binarias ===========
	
	// a_ei
	IloArray<IloNumVarArray> a(env);
	
	for(int e = 0; e < dados.quantidadeAlunos; e++){
		a.add(IloNumVarArray(env));
		
	    for(auto p : dados.alunosParadas[e].paradasPossiveis){
			a[e].add(IloIntVar(env, 0, 1));
	        numberVar++;
	    }
	}
	
	//  tr^k  (rota r do onibus k esta ativa)
	IloArray<IloNumVarArray> t(env);
	
	for(int k = 0; k < dados.quantidadeOnibus; k++){
		t.add(IloNumVarArray(env));
		
		for(int r = 0; r < dados.quantidadeRotas; r++){
			t[k].add(IloIntVar(env, 0, 1));
			numberVar++;
		}
	}

	// x^kr-st_ij : arco i->j usado no passo "st" da rota r do onibus k.
	IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>> x(env);
	for(int k = 0; k < dados.quantidadeOnibus; k++){
		x.add(IloArray<IloArray<IloArray<IloNumVarArray>>>(env));

		for(int r = 0; r < dados.quantidadeRotas; r++){
			x[k].add(IloArray<IloArray<IloNumVarArray>>(env));

			for(int st = 0; st < dados.quantidadePassos; st++){
				x[k][r].add(IloArray<IloNumVarArray>(env));

				for(int i = 0; i < dados.quantidadeParadas; i++){
					x[k][r][st].add(IloNumVarArray(env));

					for(int j = 0; j < dados.quantidadeParadas; j++){

                        // aresta inexistente, alto-cilco, menos na origem. precisa de loop na escola pra consumir o step
						if((dados.grafoParadas[i][j] == 0 && i != j) || (i == j && i != 0)){
							x[k][r][st][i].add(IloIntVar(env, 0, 0)); // Trava em 0
						} else {
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
    for(int k = 0; k < dados.quantidadeOnibus; k++){
        p.add(IloArray<IloNumVarArray>(env));

        for(int r = 0; r < dados.quantidadeRotas; r++){
            p[k].add(IloNumVarArray(env));

            for(int i = 0; i < dados.quantidadeParadas; i++){
                p[k][r].add(IloIntVar(env, 0, 1));
                numberVar++;
            }
        }
    }
    
	// y_ei^kr
	IloArray<IloArray<IloArray<IloNumVarArray>>> y(env);
	for(int k = 0; k < dados.quantidadeOnibus; k++){
	    y.add(IloArray<IloArray<IloNumVarArray>>(env));

	    for(int r = 0; r < dados.quantidadeRotas; r++){
	        y[k].add(IloArray<IloNumVarArray>(env));
            
	        for(int e = 0; e < dados.quantidadeAlunos; e++){
                y[k][r].add(IloNumVarArray(env));
                
	            for(int p = 0; p < dados.alunosParadas[e].paradasPossiveis.size(); p++){
                    y[k][r][e].add(IloIntVar(env,0,1));
                    numberVar++;
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
    for(int k = 0; k < dados.quantidadeOnibus; k++){
		for(int r = 0; r < dados.quantidadeRotas; r++ ){
			for(int st = 0; st < dados.quantidadePassos; st++){
				for(int i = 0; i < dados.quantidadeParadas; i++){
					for(int j = 0;  j < dados.quantidadeParadas; j++){
						obj1 += (dados.grafoParadas[i][j] * x[k][r][st][i][j]);
					}
				}
			}
        }
	}

    // Restrição 2 (quantidade de parada)
    for(int i = 0;  i < dados.quantidadeParadas; i++){
		obj2 += (b[i]);
    }
	
	//RESTRICOES ---------------------------------------------	

    IloExpr soma(env); 

    // --- ALOCAÇÃO DE ESTUDANTES ÀS PARADAS ---
    
    // Cada estudante deve ser alocado a exatamente uma parada valida
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        soma.clear();
        for(int p = 0; p < dados.alunosParadas[e].paradasPossiveis.size(); p++) {
            soma += a[e][p];
        }
        model.add(soma == 1);
        numberRes++;
    }

    // Um estudante só pode ser alocado a uma parada se ela estiver ativa
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        for(int p = 0; p < dados.alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = dados.alunosParadas[e].paradasPossiveis[p].first;
            model.add(a[e][p] <= b[i]);
            numberRes++;
        }
    }

    // Define W como a maior distância de caminhada
    for(int e = 0; e < dados.quantidadeAlunos; e++) {
        for(int p = 0; p < dados.alunosParadas[e].paradasPossiveis.size(); p++) {
            int i = dados.alunosParadas[e].paradasPossiveis[p].first;
            double d_ei = dados.alunosParadas[e].paradasPossiveis[p].second;
            model.add(d_ei * a[e][p] <= W);
            numberRes++;
        }
    }

    // ======== CONSERVAÇÃO DE FLUXO E ROTEAMENTO (Step) ========
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {

            // Encadeamento entre passos consecutivos: (nó de chegada do passo st) == (no de saida do passo st+1), para cada nó j
            for(int st = 0; st < dados.quantidadePassos - 1; st++) {
                for(int j = 0; j < dados.quantidadeParadas; j++) {
                    IloExpr chegadaEm_j_no_passo_st(env);
                    IloExpr saidaDe_j_no_passo_stMais1(env);

                    for(int i = 0; i < dados.quantidadeParadas; i++) {
                        chegadaEm_j_no_passo_st += x[k][r][st][i][j];
                    }
                    for(int i = 0; i < dados.quantidadeParadas; i++) {
                        saidaDe_j_no_passo_stMais1 += x[k][r][st + 1][j][i];
                    }

                    model.add(chegadaEm_j_no_passo_st == saidaDe_j_no_passo_stMais1);
                    numberRes++;

                    chegadaEm_j_no_passo_st.end();
                    saidaDe_j_no_passo_stMais1.end();
                }
            }
            

            // No maximo um arco ativo por passo: soma de todos os arcos do passo st <= 1
            for(int st = 0; st < dados.quantidadePassos; st++) {
                soma.clear();
                for(int i = 0; i < dados.quantidadeParadas; i++) {
                    for(int j = 0; j < dados.quantidadeParadas; j++) {
                        soma += x[k][r][st][i][j];
                    }
                }
                model.add(soma <= 1);
                numberRes++;
            }

            // O self-loop em 0 (x[k][r][st][0][0]) fica de fora dessa soma de proposito: 
            // ele representa a rota inativa/ociosa, entao nao pode ser limitado por t
            for(int st = 0; st < dados.quantidadePassos; st++) {
                soma.clear();
                for(int i = 0; i < dados.quantidadeParadas; i++) {
                    for(int j = 0; j < dados.quantidadeParadas; j++) {
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
            for(int j = 1; j < dados.quantidadeParadas; j++) {
                soma += x[k][r][0][0][j];
            }

            model.add(soma == t[k][r]);
            numberRes++;

            model.add(x[k][r][0][0][0] == 1 - t[k][r]);
            numberRes++;

            // A escola só pode ser ponto de partida real no passo 0.
            // Nos demais passos, se a rota ja voltou, ela deve permanecer em self-loop
            for(int st = 1; st < dados.quantidadePassos; st++){
                soma.clear();
                for(int j = 1; j < dados.quantidadeParadas; j++){
                    soma += x[k][r][st][0][j];
                }
                model.add(soma == 0);
                numberRes++;
            }

            // A rota deve retornar a escola (0) pelo menos uma vez, vindo de um no diferente de 0 (chegada de verdade, não self-loop), em algum passo
            soma.clear();
            for(int st = 0; st < dados.quantidadePassos; st++) {
                for(int i = 1; i < dados.quantidadeParadas; i++) {
                    soma += x[k][r][st][i][0];
                }
            }
            model.add(soma == t[k][r]); 
            numberRes++;
        }
    }

    // ============= ATIVAÇÃO E ASSOCIAÇÃO DE PARADAS =============

    // Vincula s[k][r][i] a "a parada i foi visitada em algum passo".
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            for(int i = 0; i < dados.quantidadeParadas; i++) {
                soma.clear();
                for(int st = 0; st < dados.quantidadePassos; st++) {
                    for(int j = 0; j < dados.quantidadeParadas; j++) {
                        soma += x[k][r][st][j][i];
                    }
                }
                model.add(soma <= dados.quantidadePassos * p[k][r][i]);
                numberRes++;

                model.add(p[k][r][i] <= soma);
                numberRes++;
            }
        }
    }

    // Se a parada b_i está ativa, pelo menos uma rota deve visitá-la
    for(int i = 0; i < dados.quantidadeParadas; i++) {
        soma.clear();
        for(int k = 0; k < dados.quantidadeOnibus; k++) {
            for(int r = 0; r < dados.quantidadeRotas; r++) {
                soma += p[k][r][i];
            }
        }
        model.add(soma >= b[i]);
        numberRes++;
    }

    // ============= CAPACIDADE DOS VEÍCULOS =============

    // Limita a quantidade de estudantes atendidos na rota à capacidade máxima Q 
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            soma.clear();
            for(int e = 0; e < dados.quantidadeAlunos; e++) {
                for(int p = 0; p < dados.alunosParadas[e].paradasPossiveis.size(); p++) {
                    soma += y[k][r][e][p];
                }
            }
            
            model.add(soma <= dados.Q * t[k][r]); 
            numberRes++;
        }
    }
    
    // ============= LINEARIZAÇÃO DA VARIÁVEL AUXILIAR Y (y = a * p) =============
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            for(int e = 0; e < dados.quantidadeAlunos; e++) {
                for(int pp = 0; pp < dados.alunosParadas[e].paradasPossiveis.size(); pp++) {

                    int i = dados.alunosParadas[e].paradasPossiveis[pp].first;

                    // Um aluno só pode ser colocado em uma rota
                    // se estiver alocado àquela parada.
                    for(int k = 0; k < dados.quantidadeOnibus; k++) {
                        for(int r = 0; r < dados.quantidadeRotas; r++) {

                            model.add(y[k][r][e][pp] <= a[e][pp]);
                            model.add(y[k][r][e][pp] <= p[k][r][i]);

                            numberRes += 2;
                        }
                    }

                    // Se o aluno escolheu essa parada, ele deve ser
                    // transportado por exatamente uma das rotas que a atende.
                    soma.clear();
                    for(int k = 0; k < dados.quantidadeOnibus; k++) {
                        for(int r = 0; r < dados.quantidadeRotas; r++) {
                            soma += y[k][r][e][pp];
                        }
                    }

                    model.add(soma == a[e][pp]);
                    numberRes++;
                }
            }
        }
    }

    // ============= RELAÇÃO ÔNIBUS-ROTA =============
    // Ativa z_k se o ônibus k operar ao menos uma rota
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        soma.clear();
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            soma += t[k][r];
        }
        model.add(soma >= z[k]);
        numberRes++;
    }

    // Impede rotas em ônibus desativados
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        for(int r = 0; r < dados.quantidadeRotas; r++) {
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
    for(int r = 1; r < dados.quantidadeRotas; r++){
        IloExpr usoAtual(env);
        IloExpr usoAnterior(env);

        for(int k = 0; k < dados.quantidadeOnibus; k++){
            usoAtual    += t[k][r];
            usoAnterior += t[k][r - 1];
        }

        model.add(usoAtual <= usoAnterior);
        numberRes++;

        usoAtual.end();
        usoAnterior.end();
    }

    // Força o uso dos onibus em ordem: oni k -> oni k+1
    for(int k = 1; k < dados.quantidadeOnibus; k++){
        model.add(z[k] <= z[k - 1]);
        numberRes++;
    }



    // ============= BALANCEAMENTO =============

    // M
    for(int k = 0; k < dados.quantidadeOnibus; k++) {
        soma.clear();
        for(int r = 0; r < dados.quantidadeRotas; r++) {
            soma += t[k][r];
        }
        model.add(M >= soma);
        numberRes++;
    }

    soma.end();
    
    
    IloCplex cplex(env);
    cplex.extract(model);


    // ============================================================
    // WARM START
    // ============================================================

    bool warmStartHab = (solucao != nullptr);

    IloNumVarArray vars(env);
    IloNumArray vals(env);

    if(warmStartHab) {
        warmStartHab = montarWarmStart(solucao, env, a, b, z, t, x, p, y, W, M, vars, vals);
        if(!warmStartHab)
            cerr << "\n[warmstart] construcao falhou. seguindo sem warm start.\n\n";
    }


  
    // ==================================== //
    // ==================================== //
	//------ EXECUCAO do MODELO ----------
    // time_t timer, timer2;
    IloNum objValue;
    double runTime;
    string status;
    
    printf("--------Informacoes da Execucao:----------\n\n");
    printf("#Var: %d\n", numberVar);
    printf("#Restricoes: %d\n", numberRes);
    cout << "Memory usage after variable creation:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;
    
    cout << "Memory usage after cplex extraction:  " << env.getMemoryUsage() / (1024. * 1024.) << " MB" << endl;

    cplex.setParam(IloCplex::TiLim, CPLEX_TIME_LIM);
    
    time_t tInicio, tFimFase1, tFimFase2, tFimFase3, tFimFase4;

    // ==============
    time(&tInicio);
    // ==============

    // ================= FASE 1: Minimizar Custo (obj1) =================
    IloObjective FO1 = IloMinimize(env, obj1);
    model.add(FO1);
    cplex.extract(model); // Atualiza o modelo no solver
    
    if(warmStartHab){ 

                
        cout << "tentando Init\n"; fflush(stdin);
        cout << "Total vars no start: " << vars.getSize() << endl;
        cout << "Total vars no modelo: " << numberVar << endl;
        
        // cplex.setParam(IloCplex::Param::MIP::Display, 3);

        // cplex.addMIPStart(vars, vals, IloCplex::MIPStartSolveFixed); 
        bool ok = cplex.addMIPStart(vars, vals, IloCplex::MIPStartRepair); 
        cout << "MIP start aceito pelo addMIPStart: " << ok << endl;
        // cplex.writeMIPStarts("checkstart.mst");
                
        // IloCplex::MIPStartCheckFeas	Só verifica se a solução (completa) é factível, não tenta consertar
        // IloCplex::MIPStartSolveFixed	Fixa as variáveis fornecidas, resolve o restante como LP/MIP menor
        // IloCplex::MIPStartSolveMIP	Resolve um sub-MIP nas variáveis não fornecidas
        // IloCplex::MIPStartRepair	    Tenta reparar automaticamente uma solução inconsistente/infactível
        // IloCplex::MIPStartNoCheck	Aceita sem verificar nada (mais arriscado)
        // IloCplex::MIPStartAuto
            
        env.out() << "MIP starts: " << cplex.getNMIPStarts() << std::endl;
        vars.end(); vals.end(); 
        
    } 



    if(!cplex.solve()) {cerr << "Erro: Fase 1 inviável!\n"; return;}
	
    double melhorCusto = cplex.getObjValue();
    model.remove(FO1);
    model.add(obj1 <= melhorCusto + 1e-4); 
    // return;
    
    // ==============
    time(&tFimFase1);
    // ==============

    // ================= FASE 2: Minimizar Qtd Paradas (obj2) =================
    IloObjective FO2 = IloMinimize(env, obj2);
    model.add(FO2);
    cplex.extract(model);
    if(!cplex.solve()) {
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
    if(!cplex.solve()) {
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
        cout << "\n\n\n\n\n\n\n";

        printf("\n================= CENÁRIO DE ENTRADA =================\n");
        printf("Paradas (incluindo escola 0): %d\n", dados.quantidadeParadas);
        printf("Estudantes: %d\n", dados.quantidadeAlunos);
        printf("Onibus disponiveis: %d\n", dados.quantidadeOnibus);
        printf("Rotas possiveis por onibus (limite superior): %d\n", dados.quantidadeRotas);
        printf("Capacidade maxima por onibus (Q): %d\n", dados.Q);
        printf("Passos maximos por rota: %d\n", dados.quantidadePassos);
        printf("Arestas no grafo de paradas: %d\n", dados.quantidadeArestas);
        printf("Maior distancia aluno-parada no arquivo: %d\n", dados.maxDistancia);

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

        for(int k = 0; k < dados.quantidadeOnibus; k++){
            if(cplex.getValue(z[k]) < 0.5) continue;

            cout << "\n>> Onibus " << k << "\n";

            for(int r = 0; r < dados.quantidadeRotas; r++){
                if(cplex.getValue(t[k][r]) < 0.5) continue;

                // reconstroi o caminho e calcula custo/paradas da rota
                vector<int> caminho = {0};
                double custoRota = 0;

                for(int st = 0; st < dados.quantidadePassos; st++){
                    int de = -1, para = -1;

                    for(int i = 0; i < dados.quantidadeParadas && de == -1; i++){
                        for(int j = 0; j < dados.quantidadeParadas; j++){
                            double val = cplex.getValue(x[k][r][st][i][j]);
                            if(IloRound(val) >= 1){ de = i; para = j; break; }
                        }
                    }

                    if(de == -1) continue;      // passo sem arco detectado, segue tentando
                    if(de == 0 && para == 0) continue; // self-loop ocioso

                    custoRota += dados.grafoParadas[de][para];
                    caminho.push_back(para);
                }

                // conta alunos e ocupacao dessa rota
                int alunosNaRota = 0;
                vector<string> passageiros;
                for(int e = 0; e < dados.quantidadeAlunos; e++){
                    for(int p = 0; p < (int) dados.alunosParadas[e].paradasPossiveis.size(); p++){
                        if(cplex.getValue(y[k][r][e][p]) > 0.5){
                            alunosNaRota++;
                            passageiros.push_back(
                                "Aluno " + to_string(dados.alunosParadas[e].id) +
                                " (parada " + to_string(dados.alunosParadas[e].paradasPossiveis[p].first) + ")");
                        }
                    }
                }

                // paradas visitadas por essa rota
                vector<int> paradasRota;
                for(int i = 0; i < dados.quantidadeParadas; i++)
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
                cout << "     Ocupacao: " << alunosNaRota << " / " << dados.Q << " alunos"
                     << " (" << (100.0 * alunosNaRota / dados.Q) << "% da capacidade)\n";
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
        for(int i = 0; i < dados.quantidadeParadas; i++){
            if(cplex.getValue(b[i]) > 0.5) paradasUsadas.push_back(i);
            else paradasNaoUsadas.push_back(i);
        }

        cout << "Usadas (" << paradasUsadas.size() << "/" << dados.quantidadeParadas << "): ";
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

        for(int e = 0; e < dados.quantidadeAlunos; e++){
            bool encontrou = false;

            for(int p = 0; p < (int) dados.alunosParadas[e].paradasPossiveis.size(); p++){
                if(cplex.getValue(a[e][p]) > 0.5){

                    int parada = dados.alunosParadas[e].paradasPossiveis[p].first;
                    int distancia = dados.alunosParadas[e].paradasPossiveis[p].second;

                    somaDist += distancia;
                    maiorDist = max(maiorDist, (double) distancia);

                    cout << "  Aluno " << dados.alunosParadas[e].id
                         << " -> Parada " << parada
                         << " (caminhada: " << distancia << ")";

                    for(int k = 0; k < dados.quantidadeOnibus; k++){
                        for(int r = 0; r < dados.quantidadeRotas; r++){
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
                cout << "  Aluno " << dados.alunosParadas[e].id << " -> NAO ALOCADO !!\n";
            }
        }

        cout << "\nResumo de caminhada: media = "
             << (dados.quantidadeAlunos - naoAlocados > 0 ? somaDist / (dados.quantidadeAlunos - naoAlocados) : 0)
             << " | maior individual = " << maiorDist
             << " | alunos nao alocados = " << naoAlocados << "\n\n";

        // ======================================================
        // 4] ONIBUS - RESUMO
        // ======================================================
        cout << "[4] ONIBUS\n";
        cout << "--------------------------------------------------------\n";

        for(int k = 0; k < dados.quantidadeOnibus; k++){
            int rotasDoOnibus = 0;
            for(int r = 0; r < dados.quantidadeRotas; r++)
                if(cplex.getValue(t[k][r]) > 0.5) rotasDoOnibus++;

            cout << "  Onibus " << k << ": "
                 << (cplex.getValue(z[k]) > 0.5 ? "UTILIZADO" : "nao utilizado")
                 << " | rotas atribuidas: " << rotasDoOnibus << "\n";
        }

        cout << "\n";
        cout << "========================================================\n";
        cout << "                   RESUMO GERAL\n";
        cout << "========================================================\n";
        cout << "Alunos atendidos:      " << (dados.quantidadeAlunos - naoAlocados) << " / " << dados.quantidadeAlunos << "\n";
        cout << "Paradas ativas:        " << paradasUsadas.size() << " / " << dados.quantidadeParadas << "\n";
        cout << "Onibus utilizados:     ";
       
        int cont = 0;
        for(int k = 0; k < dados.quantidadeOnibus; k++) if(cplex.getValue(z[k]) > 0.5) cont++;
        cout << cont << " / " << dados.quantidadeOnibus << "\n";
    
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
