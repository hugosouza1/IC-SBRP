#include "SBRP.hpp"


void infoSBRP::leitura(string arquivoEntrada){
    ifstream arq(arquivoEntrada);
    if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1);}

    arq >> quantidadeParadas >> quantidadeAlunos >> quantidadeOnibus >> Q;

    quantidadeRotas = ((quantidadeAlunos + Q) / Q) + 2; // caiu de 4min pra 30s

    quantidadePassos = quantidadeParadas; // de 30 do de cima pra 9s. diliça
    quantidadePassos = quantidadeParadas * quantidadeOnibus * 2; // de 30 do de cima pra 9s. diliça

    int maxx = numeric_limits<int>::max();
    grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, maxx));
    for(int i = 0; i < quantidadeParadas; i++){
        grafoParadas[i][i] = 0;
    }

    arq >> quantidadeArestas;
    for(int i = 0; i < quantidadeArestas; i++){
        int pontoA, pontoB, peso; 
        arq >> pontoA >> pontoB >> peso;
        grafoParadas[pontoA][pontoB] = min(grafoParadas[pontoA][pontoB], peso);
        grafoParadas[pontoB][pontoA] = min(grafoParadas[pontoB][pontoA], peso);
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
