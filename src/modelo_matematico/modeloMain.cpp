#include "modelo.hpp"

void handler(int sinal) {
    std::cerr << "Sinal recebido: " << sinal << std::endl;
}

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    std::signal(SIGHUP, handler);

    infoSBRP dados;
    
    dados.leitura(argc[1]);

    cout << "\nLeitura de dados concluida\n\n"; fflush(stdin);
    
    ModeloMatematico modMAt(dados);
    modMAt.cplexSolver();


    return 0;
}