#pragma once

#include <vector>
#include <queue>
#include <utility>
#include <functional>

inline constexpr double INFINITO = 1e18;

// Fecho métrico: caminho mínimo entre quaisquer paradas, calculado sob demanda (Dijkstra por origem).
// Header-only (inline) para não mexer no build.
struct DistanciasSobDemanda {

    std::vector<std::vector<double>> grafo; // 0 = sem aresta
    int quantidadeParadas = 0;

    mutable std::vector<std::vector<double>> distancia;
    mutable std::vector<std::vector<int>> predecessor;
    mutable std::vector<char> calculada;

    void inicia(const std::vector<std::vector<double>>& g){
        grafo = g;
        quantidadeParadas = g.size();
        distancia.assign(quantidadeParadas, {});
        predecessor.assign(quantidadeParadas, {});
        calculada.assign(quantidadeParadas, 0);
    }

    const std::vector<double>& operator[](int origem) const {
        if(!calculada[origem]) dijkstra(origem);
        return distancia[origem];
    }

    const std::vector<int>& predecessoresDe(int origem) const {
        if(!calculada[origem]) dijkstra(origem);
        return predecessor[origem];
    }

    void dijkstra(int origem) const {
        std::vector<double>& dist = distancia[origem];
        std::vector<int>& pai = predecessor[origem];

        dist.assign(quantidadeParadas, INFINITO);
        pai.assign(quantidadeParadas, -1);
        dist[origem] = 0;

        std::priority_queue<std::pair<double,int>,
                            std::vector<std::pair<double,int>>,
                            std::greater<std::pair<double,int>>> fila;
        fila.push({0.0, origem});

        while(!fila.empty()){
            auto [du, u] = fila.top(); fila.pop();
            if(du > dist[u]) continue;

            for(int v = 0; v < quantidadeParadas; ++v){
                double peso = grafo[u][v];
                if(peso > 0 && du + peso < dist[v]){
                    dist[v] = du + peso;
                    pai[v] = u;
                    fila.push({dist[v], v});
                }
            }
        }
        calculada[origem] = 1;
    }
};