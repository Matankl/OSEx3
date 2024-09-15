#include <iostream>
#include <deque>
#include <vector>
#include <algorithm>
#include <sstream>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

class Graph {
    int n;  // Number of vertices
    vector<deque<int>> adj;  // Adjacency list using deque

public:
    Graph(int vertices = 0) : n(vertices) {
        adj.resize(n + 1);  // Resize adjacency list for 1-based indexing
    }

    void addEdge(int u, int v) {
        adj[u].push_back(v);  // Add edge u -> v in the original graph
    }

    void removeEdge(int u, int v) {
        auto it = find(adj[u].begin(), adj[u].end(), v);
        if (it != adj[u].end()) {
            adj[u].erase(it);  // Erase the edge v from the adjacency list of u
        }
    }

    int getNumVertices() const {
        return n;
    }

    const deque<int>& getAdjList(int v) const {
        return adj[v];
    }

    Graph transposeGraph() const {
        Graph transposed(n);
        for (int u = 1; u <= n; ++u) {
            for (int v : adj[u]) {
                transposed.addEdge(v, u);
            }
        }
        return transposed;
    }
};

mutex graph_mutex;

void printSCCs(const Graph &g) {
    int n = g.getNumVertices();
    deque<int> Stack;
    vector<bool> visited(n + 1, false);

    auto fillOrder = [&](int v, deque<int> &Stack) {
        visited[v] = true;
        for (int i : g.getAdjList(v)) {
            if (!visited[i]) {
                fillOrder(i, Stack);
            }
        }
        Stack.push_back(v);
    };

    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            fillOrder(i, Stack);
        }
    }

    Graph transposed = g.transposeGraph();
    fill(visited.begin(), visited.end(), false);

    while (!Stack.empty()) {
        int v = Stack.back();
        Stack.pop_back();
        if (!visited[v]) {
            deque<int> component;
            auto dfs = [&](int v, deque<int> &component) {
                visited[v] = true;
                component.push_back(v);
                for (int i : transposed.getAdjList(v)) {
                    if (!visited[i]) {
                        dfs(i, component);
                    }
                }
            };
            dfs(v, component);
            for (int vertex : component) {
                cout << vertex << " ";
            }
            cout << endl;
        }
    }
}

void handleClient(int client_socket) {
    Graph g;
    char buffer[1024] = {0};
    while (true) {
        read(client_socket, buffer, 1024);
        string command(buffer);
        if (command == "Exit") {
            close(client_socket);
            break;
        }

        stringstream ss(command);
        string option;
        ss >> option;

        if (option == "Newgraph") {
            int n, m;
            ss >> n >> m;
            lock_guard<mutex> lock(graph_mutex);
            g = Graph(n);
            for (int i = 0; i < m; ++i) {
                int u, v;
                ss >> u >> v;
                g.addEdge(u, v);
            }
        } else if (option == "Kosaraju") {
            lock_guard<mutex> lock(graph_mutex);
            printSCCs(g);
        } else if (option == "Newedge") {
            int u, v;
            ss >> u >> v;
            lock_guard<mutex> lock(graph_mutex);
            g.addEdge(u, v);
        } else if (option == "Removeedge") {
            int u, v;
            ss >> u >> v;
            lock_guard<mutex> lock(graph_mutex);
            g.removeEdge(u, v);
        } else {
            cout << "Invalid command: " << command << endl;
        }
        memset(buffer, 0, sizeof(buffer));
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9037);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        thread t(handleClient, new_socket);
        t.detach();
    }

    return 0;
}
