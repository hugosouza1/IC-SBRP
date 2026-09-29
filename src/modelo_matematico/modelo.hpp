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

        void cplexSolver();        
};