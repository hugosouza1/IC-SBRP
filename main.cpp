#include "SBRP.hpp"
#include "metaheuristica/metaheuristica.hpp"

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;

    dados.leitura(argc[1]);


    cout << "\nLeu dados\n\n"; fflush(stdin);
    
    
    Metaheuristica meta(dados);
    // Individuo teste;

    Individuo teste = meta.AG();
    
    
    
    // teste = meta.geraSolucaoInicial();

    cout << "\Solução Inicial\n\n"; fflush(stdin);
    
    // meta.buscaTabu(teste);

    // cout << "\\n\n"; fflush(stdin);

    cout << "teste fit: ";
    cout << teste.fitness << "\n";
    
    cout << "\n";
    for(int i = 0; i < teste.rotasFeitas.size(); i ++){
        cout << "\nrota " << i << ":\n";
        for(int j = 0; j < teste.rotasFeitas[i].size(); j++){
            cout << " " << teste.rotasFeitas[i][j];
        }
        cout << "\n";
    }

    // dados.cplex();

    return 0;
}