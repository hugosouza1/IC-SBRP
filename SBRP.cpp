#include "SBRP.hpp"


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
                int alunoId, parada;
                double distancia;
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
    quantidadeRotas   = (((quantidadeAlunos + Q) / Q) + 1) * 2;
    quantidadePassos  = quantidadeParadas * 1.5;
    arq.close();
}

