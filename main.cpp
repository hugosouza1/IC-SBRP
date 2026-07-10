#include "SBRP.hpp"
#include "metaheuristica/metaheuristica.hpp"

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;

    dados.leitura(argc[1]);

    Metaheuristica meta(dados);

    Individuo teste;

    teste = meta.geraSolucaoInicial();

    meta.buscaTabu(teste);

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