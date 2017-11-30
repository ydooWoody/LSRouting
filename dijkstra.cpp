#include <iostream>
#include <vector>
#include <set>
#include <limits>
using namespace std;

const int SIZE = 1000;

class Graph {
    int g[SIZE][SIZE];

public:
    void add(int x, int y, int w) {
        g[x][y] = w;
        g[y][x] = w;
    }

    int dijkstra(int src, int dest) {
        set<int> q;
        vector<int> dist(SIZE);
        //vector<int> prev(SIZE);

        for (int v = 0; v < SIZE; ++v) {
            dist[v] = numeric_limits<int>::max();
            //prev[v] = -1;
            q.insert(v);
        }
        dist[src] = 0;

        while (!q.empty()) {
            int u = *q.begin();
            int min_dist = dist[*q.begin()];
            for (int v : q) {
                if (dist[v] < min_dist) {
                    min_dist = dist[v];
                    u = v;
                }
            }
            q.erase(u);

            for (int v = 0; v < SIZE; ++v) {
                if (g[u][v] != 0 && q.count(v)) {
                    int new_dist = dist[u] + g[u][v];
                    if (new_dist < dist[v]) {
                        dist[v] = new_dist;
                        //prev[v] = u;
                    }
                }
            }
        }
        return dist[dest];
    }
};

int main() {
    Graph graph;
    graph.add(0, 1, 10);
    graph.add(0, 3, 5);
    graph.add(1, 2, 1);
    graph.add(3, 2, 2);
    cout << graph.dijkstra(0, 2) << endl; // should be 7
    return 0;
}