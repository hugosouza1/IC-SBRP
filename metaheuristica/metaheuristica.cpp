#include "metaheuristica.hpp"
#include "../SBRP.hpp"

double desvioPadrao(const vector<double>& valores) {
    if (valores.empty())
        return 0.0;

    double soma = 0.0;

    for (double x : valores)
        soma += x;

    double media = soma / valores.size();

    double somaQuadrados = 0.0;

    for (double x : valores)
        somaQuadrados += pow(x - media, 2);

    double variancia = somaQuadrados / valores.size();

    return sqrt(variancia);
}

int main(int argv, char *argc[]){
    if(argv < 2){
        cerr << "ERRO: falta nome do arquivo de vertices e alunos\n";
    }

    infoSBRP dados;
    dados.leitura(argc[1]);

    cout << "\nLeitura de dados concluida: " << argc[1] << "\n\n"; fflush(stdin);
    
    Metaheuristica meta(dados);


    {
        cout << "\nAG-BT:\n";
        auto [valoresGeracoes, ciclano]  = meta.AG(1);

        meta.imprimeSolucao(ciclano, dados, valoresGeracoes);
    }

    






    return 0;
     
    {
        cout << "\nAG:\n";
        auto [fulano, valoresGeracoes] = meta.AG(0);
    }
    
    {
        cout << "\nBT:\n";
        int geracion = 100;
        Individuo melhorInd;
        vector<double> valores;
        for(int i = 0; i < geracion; ++i){
            Individuo beltrano = meta.geraSolucaoInicial();   
            meta.buscaTabu(beltrano);
            if(beltrano.fitness < melhorInd.fitness){
                melhorInd = beltrano;
            }
            cout << beltrano.fitness << "\t";
            if(i%16==0){
                cout << "\n";
            }
            
            valores.push_back(beltrano.fitness);
        }
        
        double dp = desvioPadrao(valores);
        double media = accumulate(valores.begin(), valores.end(), 0);
        
        cout << "\nMelhor valor: " << melhorInd.fitness << "\n";
        cout << "Desvio Padrao: " << dp << "\nMedia: " << media / valores.size() << "\n";
    }

    // cout << "\nMetaheuristica concluida\n\n"; fflush(stdin);
    
    return 0;
}



















void Metaheuristica::imprimeSolucao(Individuo& sol, infoSBRP& dados, vector<double> valores){
    cout << "\n========================================\n";
    cout << "        SOLUCAO - AG-BT (SBRP)\n";
    cout << "========================================\n\n";
    cout << "Distancia total: " << sol.fitness - sol.penalidadeFitness << " [penalidade:" << sol.penalidadeFitness << "]" << "\n";
    cout << "Desvio Padrão: " << desvioPadrao(valores) << "\n";

    // agrupa os alunos por rota original (antes de renumerar)
    vector<vector<int>> alunosPorRota(sol.rotasFeitas.size());
    for(int aluno = 0; aluno < dados.quantidadeAlunos; aluno++){
        int rota = sol.atrAlunoRota[aluno];
        alunosPorRota[rota].push_back(aluno);
    }

    int rotaImpressa = 0;
    for(int r = 0; r < (int)sol.rotasFeitas.size(); r++){

        if(sol.rotasFeitas[r].empty()) continue;

        int pesoRota = 0;
        for(int i = 0; i + 1 < (int)sol.rotasFeitas[r].size(); i++){
            pesoRota += dados.grafoParadas[sol.rotasFeitas[r][i]][sol.rotasFeitas[r][i+1]];
        }

        cout << "\n\n---- Rota " << rotaImpressa << " ----\n";
        cout << "Ônibus atribuído: " << sol.atrOnibusRota[rotaImpressa] << "(capacidade: " << problema.capacidadeOnibus[sol.atrOnibusRota[rotaImpressa]] << ")\n";
        cout << "Peso da rota: " << pesoRota << "\n";

        cout << "Trajeto (paradas): ";
        for(int i = 0; i < (int)sol.rotasFeitas[r].size(); i++){
            cout << sol.rotasFeitas[r][i];
            if(i + 1 < (int)sol.rotasFeitas[r].size()) cout << " -> ";
        }
        cout << "\n";

        cout << "Estudantes (" << alunosPorRota[r].size() << "): ";
        int it = 0;
        cout << "[parada->alunos]\n";
        set<int> para;

        for(int aluno : alunosPorRota[r]){
            para.insert(sol.atrAlunoParada[aluno]);
        }
        
        for(int parad : para){
            cout << "\t\t[" << parad << " -> ";
            bool pri = true;
            for(int aluno : alunosPorRota[r]){
                if(parad == sol.atrAlunoParada[aluno]){
                    if(pri){   
                        cout << aluno << "";
                        pri = false;
                    }
                    else cout << ", " << aluno;

                }
                // cout << "[" << aluno << "->" << sol.atrAlunoParada[aluno] << "], ";
            }
            cout << "]\n";
        }

        rotaImpressa++;
    }

    cout << "\n----------------------------------------\n";
    cout << "Total de onibus usados: " << rotaImpressa << "\n";
    cout << "========================================\n";
}