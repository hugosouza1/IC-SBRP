#include "SBRP.hpp"


// void infoSBRP::leitura(string arquivoEntrada){
//     ifstream arq(arquivoEntrada);
//     if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1);}

//     arq >> quantidadeParadas >> quantidadeAlunos >> quantidadeOnibus >> Q;

//     // quantidadeRotas = 5; 
//     quantidadeRotas = ((quantidadeAlunos + Q) / Q) + 1; // caiu de 4min pra 30s

//     quantidadePassos = quantidadeParadas * 1.5; 
//     // quantidadePassos = quantidadeParadas * quantidadeOnibus * 1.5; 

//     // int maxx = numeric_limits<int>::max();
//     // grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, maxx));
    
//     grafoParadas.assign(quantidadeParadas, vector<int>(quantidadeParadas, 0));
    
//     for(int i = 0; i < quantidadeParadas; i++){
//         grafoParadas[i][i] = 0; // preciso de 0 pra auto-loop do modelo
//     }

//     arq >> quantidadeArestas;
//     for(int i = 0; i < quantidadeArestas; i++){
//         int pontoA, pontoB, peso; 
//         arq >> pontoA >> pontoB >> peso;
//         grafoParadas[pontoA][pontoB] = max(grafoParadas[pontoA][pontoB], peso);
//         grafoParadas[pontoB][pontoA] = max(grafoParadas[pontoB][pontoA], peso);
//     }


//     alunosParadas.clear();
//     for(int e = 0; e < quantidadeAlunos; e++){
//         estudante aluno;
//         int quantidadeParadasPossiveis;

//         /*
//             exemplo:

//             0 2
//             1 10
//             3 15

//             aluno 0
//             possui 2 paradas possíveis
//         */

//         arq >> aluno.id;
//         arq >> quantidadeParadasPossiveis;

//         for(int j = 0; j < quantidadeParadasPossiveis; j++){
//             int parada;
//             int distancia;

//             arq >> parada >> distancia;

//             aluno.paradasPossiveis.push_back({ parada, distancia });

//             // if(distancia > maxDistancia){
//             //     maxDistancia = distancia;
//             // }
//         }

//         alunosParadas.push_back(aluno);
//     }

//     arq.close();
// }



void infoSBRP::leitura(string arquivoEntrada){
    ifstream arq(arquivoEntrada);
    if(!arq.is_open()){ cerr << "Erro ao abrir arquivo\n"; exit(1); }

    enum Secao { NENHUMA, PARAMETROS, ESCOLA, PARADAS, ALUNOS, ATRIBUICOES, ONIBUS, ARESTAS };
    Secao secaoAtual = NENHUMA;

    int quantidadeParadasArquivo = 0; // paradas comuns, sem contar a escola
    int quantidadeAlunosArquivo = 0; 
    unordered_map<int,int> idParaIndice; // aluno.id -> indice em alunosParadas
    alunosParadas.clear();

    string linha;
    while(getline(arq, linha)){
        if(linha.empty()) continue;

        if(linha.rfind("PARAMETROS", 0) == 0){ secaoAtual = PARAMETROS; continue; }
        if(linha.rfind("ESCOLA", 0) == 0){ secaoAtual = ESCOLA; continue; }
        if(linha.rfind("PARADAS", 0) == 0){ secaoAtual = PARADAS; continue; }
        if(linha.rfind("ALUNOS", 0) == 0){ secaoAtual = ALUNOS; continue; }
        if(linha.rfind("ATRIBUICOES", 0) == 0){ secaoAtual = ATRIBUICOES; continue; }
        if(linha.rfind("ONIBUS", 0) == 0){ secaoAtual = ONIBUS; continue; }
        if(linha.rfind("ARESTAS", 0) == 0){
            
            quantidadeParadas = quantidadeParadasArquivo + 1; // +1 pela escola (no 0)
            grafoParadas.assign(quantidadeParadas, vector<double>(quantidadeParadas, 0));
            secaoAtual = ARESTAS;
            continue;
        }

        istringstream iss(linha);

        switch(secaoAtual){
            case PARAMETROS: {
                break;
            }

            case ESCOLA: {
                int id; double x, y;
                iss >> id >> x >> y;
                break;
            }

            case PARADAS: {
                int id; double x, y;
                iss >> id >> x >> y;
                quantidadeParadasArquivo++;
                break;
            }

            case ALUNOS: {
                estudante aluno;
                double x, y;
                iss >> aluno.id >> x >> y;
                quantidadeAlunosArquivo++;

                alunosParadas.push_back(aluno);
                break;
            }

            case ATRIBUICOES: {
                int alunoId, parada, distancia;
                iss >> alunoId >> parada >> distancia;

                alunosParadas[alunoId].paradasPossiveis.push_back({ parada, distancia });
                break;
            }

            case ONIBUS: {
                iss >> quantidadeOnibus >> Q;
                break;
            }
            case ARESTAS: {
                int pontoA, pontoB;
                double peso;
                iss >> pontoA >> pontoB >> peso;
                grafoParadas[pontoA][pontoB] = max(grafoParadas[pontoA][pontoB], peso);
                grafoParadas[pontoB][pontoA] = max(grafoParadas[pontoB][pontoA], peso);
                break;
            }
            default: break;
        }
    }

    quantidadeAlunos  = (int)alunosParadas.size();
    quantidadeRotas   = ((quantidadeAlunos + Q) / Q) + 1;
    quantidadePassos  = quantidadeParadas * 1.5;

    arq.close();
}

