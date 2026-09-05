#include "SBRP.hpp"
#include "modelo_matematico/modelo.hpp"

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;
    dados.leitura(argc[1]);

    cout << "\nLeitura de dados concluida\n\n"; fflush(stdin);
    
    // modMat.cplexSolver();
    
    return 0;
}