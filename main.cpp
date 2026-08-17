#include "SBRP.hpp"
#include "metaheuristica/metaheuristica.hpp"
#include "modelo_matematico/modelo.hpp"

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;
    dados.leitura(argc[1]);

    cout << "\nLeitura de dados concluida\n\n"; fflush(stdin);
    
    Metaheuristica meta(dados);
    Individuo fulano = meta.warmStart();

    cout << "\nMetaheuristica concluida\n\n"; fflush(stdin);

    // cout << fulano.alunosInviaveisQuant << "\n"; fflush(stdin);
    
    ModeloMatematico modMat(dados);
    modMat.cplexSolver(fulano);
    
    cout << "\nModelo Mat concluido\n\n"; fflush(stdin);
    // dados.cplex();

    return 0;
}