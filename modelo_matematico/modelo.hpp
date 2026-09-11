#pragma once

#include "../SBRP.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <cmath>

#include <bits/stdc++.h>
#include <ilcplex/ilocplex.h>


using namespace std;
ILOSTLBEGIN 

#define CPLEX_TIME_LIM 3600 //3600 segundos
// int INF = INT_MAX;

class infoSBRP;

class ModeloMatematico{
    private: 
        infoSBRP &dados;

    public:
        ModeloMatematico(infoSBRP& p) : dados(p){}

        void cplexSolver(Individuo* WarmStartConfig = nullptr);

        bool montarWarmStart(
            Individuo* solucao,
            IloEnv& env,
            IloArray<IloNumVarArray>& a,
            IloNumVarArray& b,
            IloNumVarArray& z,
            IloArray<IloNumVarArray>& t,
            IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>>& x,
            IloArray<IloArray<IloNumVarArray>>& p,
            IloArray<IloArray<IloArray<IloNumVarArray>>>& y,
            IloNumVar& W,
            IloNumVar& M,
            IloNumVarArray& vars,
            IloNumArray& vals);

        void verificarWarmStart(
            Individuo* solucao,
            const vector<bool>& rotaUsada,
            const vector<int>& rotaParaOnibus,
            IloArray<IloNumVarArray>& a,
            IloNumVarArray& b,
            IloNumVarArray& z,
            IloArray<IloNumVarArray>& t,
            IloArray<IloArray<IloArray<IloArray<IloNumVarArray>>>>& x,
            IloArray<IloArray<IloNumVarArray>>& p,
            IloArray<IloArray<IloArray<IloNumVarArray>>>& y,
            IloNumVar& W,
            IloNumVar& M,
            IloNumVarArray& vars,
            IloNumArray& vals);
        
};