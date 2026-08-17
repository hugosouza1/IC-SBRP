#include "../SBRP.hpp"

class infoSBRP;

class ModeloMatematico{
    private: 
        infoSBRP &dados;

    public:
        ModeloMatematico(infoSBRP& p) : dados(p){}

        void cplexSolver(Individuo& WarmStartConfig);
        
};