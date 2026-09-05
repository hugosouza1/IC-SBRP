#include "metaheuristica.hpp"
#include "../SBRP.hpp"


int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;
    dados.leitura(argc[1]);

    cout << "\nLeitura de dados concluida\n\n"; fflush(stdin);
    
    Metaheuristica meta(dados);
    
    cout << "AG-BT:\n";
    Individuo ciclano = meta.AG(1);
    
    cout << "AG:\n";
    Individuo fulano = meta.AG(0);
    
    cout << "BT:\n";
    Individuo beltrano = meta.geraSolucaoInicial();   
    meta.buscaTabu(beltrano);
    cout << "Melhor valor: " << beltrano.fitness << "\n";


    // cout << "\nMetaheuristica concluida\n\n"; fflush(stdin);
    
    return 0;
}

void Metaheuristica::imprimeSolucao(Individuo& sol, infoSBRP& dados){
    cout << "\n========================================\n";
    cout << "        SOLUCAO - AG-BT (SBRP)\n";
    cout << "========================================\n";
    cout << "Peso total (distancia): " << sol.fitness << "\n";

    // agrupa os alunos por rota original (antes de renumerar)
    vector<vector<int>> alunosPorRota(sol.rotasFeitas.size());
    for(int aluno = 0; aluno < dados.quantidadeAlunos; aluno++){
        int rota = sol.atrAlunoRota[aluno];
        alunosPorRota[rota].push_back(aluno);
    }

    int rotaImpressa = 0;
    for(int r = 0; r < (int)sol.rotasFeitas.size(); r++){

        if(sol.rotasFeitas[r].empty()) continue; // pula rota vazia sem deixar buraco na numeracao

        int pesoRota = 0;
        for(int i = 0; i + 1 < (int)sol.rotasFeitas[r].size(); i++){
            pesoRota += dados.grafoParadas[sol.rotasFeitas[r][i]][sol.rotasFeitas[r][i+1]];
        }

        cout << "\n---- Rota " << rotaImpressa << " ----\n";
        cout << "Peso da rota: " << pesoRota << "\n";

        cout << "Trajeto (paradas): ";
        for(int i = 0; i < (int)sol.rotasFeitas[r].size(); i++){
            cout << sol.rotasFeitas[r][i];
            if(i + 1 < (int)sol.rotasFeitas[r].size()) cout << " -> ";
        }
        cout << "\n";

        cout << "Estudantes (" << alunosPorRota[r].size() << "):\n";
        for(int aluno : alunosPorRota[r]){
            cout << "  aluno " << aluno << " -> embarca na parada " << sol.atrAlunoParada[aluno] << "\n";
        }

        rotaImpressa++;
    }

    cout << "\n----------------------------------------\n";
    cout << "Total de onibus usados: " << rotaImpressa << "\n";
    cout << "========================================\n";
}