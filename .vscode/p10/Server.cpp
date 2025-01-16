#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include <stack>

using namespace std;

// Graph class for managing the directed graph and performing operations
class Graph {
    int n;  // Number of vertices
    vector<vector<int>> adj;  // Adjacency list for the original graph

public:
    Graph(int vertices = 0) : n(vertices) {
        adj.resize(n + 1);  // Resize adjacency list for 1-based indexing
    }

    void addEdge(int u, int v) {
        adj[u].push_back(v);
    }

    void removeEdge(int u, int v) {
        adj[u].erase(remove(adj[u].begin(), adj[u].end(), v), adj[u].end());
    }

    const vector<int>& getAdjList(int v) const {
        return adj[v];
    }

    int getNumVertices() const {
        return n;
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

    void fillOrder(int v, vector<bool>& visited, stack<int>& Stack) const {
        visited[v] = true;
        for (int i : adj[v]) {
            if (!visited[i]) {
                fillOrder(i, visited, Stack);
            }
        }
        Stack.push(v);
    }

    void dfs(int v, vector<bool>& visited, vector<int>& component) const {
        visited[v] = true;
        component.push_back(v);
        for (int i : adj[v]) {
            if (!visited[i]) {
                dfs(i, visited, component);
            }
        }
    }

    vector<vector<int>> findSCCs() const {
        stack<int> Stack;
        vector<bool> visited(n + 1, false);

        // Fill the stack with vertices in the order of their finish times
        for (int i = 1; i <= n; ++i) {
            if (!visited[i]) {
                fillOrder(i, visited, Stack);
            }
        }

        // Get the transpose of the graph
        Graph transposed = transposeGraph();

        // Reset visited array for the second DFS
        fill(visited.begin(), visited.end(), false);

        vector<vector<int>> allSCCs;

        // Perform DFS on the transposed graph
        while (!Stack.empty()) {
            int v = Stack.top();
            Stack.pop();
            if (!visited[v]) {
                vector<int> component;
                transposed.dfs(v, visited, component);
                allSCCs.push_back(component);
            }
        }
        return allSCCs;
    }
};

// Shared resources
Graph graph;
pthread_mutex_t graphMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sccCond = PTHREAD_COND_INITIALIZER;
bool graphHasLargeSCC = false;

// Function to process client commands
string processCommand(int clientSocket, const string& command, unordered_map<int, pair<int, int>>& pendingEdges) {
    stringstream ss(command);
    string operation;
    ss >> operation;

    string response;

    if (operation == "Newgraph") {
        int vertices, edges;
        ss >> vertices >> edges;

        pthread_mutex_lock(&graphMutex); // Lock for thread-safe graph modification
        graph = Graph(vertices);
        pthread_mutex_unlock(&graphMutex);

        response = "Graph initialized with " + to_string(vertices) + " vertices and " + to_string(edges) + " edges.\n";

        // Add this client to the pending edges map
        pendingEdges[clientSocket] = {edges, 0}; // {total_edges, edges_added}
    } else if (pendingEdges.find(clientSocket) != pendingEdges.end()) {
        // Handle edge input for the pending "Newgraph" command
        int u, v;
        ss >> u >> v;

        pthread_mutex_lock(&graphMutex);
        graph.addEdge(u, v);
        pthread_mutex_unlock(&graphMutex);

        response = "Edge added: " + to_string(u) + " -> " + to_string(v) + "\n";

        pendingEdges[clientSocket].second++;
        if (pendingEdges[clientSocket].second == pendingEdges[clientSocket].first) {
            response += "All edges added.\n";
            pendingEdges.erase(clientSocket); // Remove client from pending edges
        }
    } else if (operation == "Newedge") {
        int u, v;
        ss >> u >> v;

        pthread_mutex_lock(&graphMutex);
        graph.addEdge(u, v);
        pthread_mutex_unlock(&graphMutex);

        response = "Edge added: " + to_string(u) + " -> " + to_string(v) + "\n";
    } else if (operation == "Removeedge") {
        int u, v;
        ss >> u >> v;

        pthread_mutex_lock(&graphMutex);
        graph.removeEdge(u, v);
        pthread_mutex_unlock(&graphMutex);

        response = "Edge removed: " + to_string(u) + " -> " + to_string(v) + "\n";
    } else if (operation == "Kosaraju") {
        vector<vector<int>> sccs;
        int totalVertices;

        pthread_mutex_lock(&graphMutex);
        sccs = graph.findSCCs();
        totalVertices = graph.getNumVertices();
        pthread_mutex_unlock(&graphMutex);

        // Find the largest SCC
        int largestSCCSize = 0;
        for (const auto& scc : sccs) {
            largestSCCSize = max(largestSCCSize, (int)scc.size());
        }

        bool currentLargeSCC = (largestSCCSize >= totalVertices / 2);

        // Notify the condition variable if the 50% condition changes
        pthread_mutex_lock(&graphMutex);
        if (currentLargeSCC != graphHasLargeSCC) {
            graphHasLargeSCC = currentLargeSCC;
            pthread_cond_broadcast(&sccCond);
        }
        pthread_mutex_unlock(&graphMutex);

        stringstream result;
        result << "Strongly Connected Components:\n";
        for (const auto& scc : sccs) {
            for (int vertex : scc) {
                result << vertex << " ";
            }
            result << "\n";
        }
        response = result.str();
    } else {
        response = "Unknown command.\n";
    }

    return response;
}

// Function to monitor SCC size and print updates
void* sccMonitor(void*) {
    bool prevLargeSCC = false;
    while (true) {
        pthread_mutex_lock(&graphMutex);
        while (graphHasLargeSCC == prevLargeSCC) {
            pthread_cond_wait(&sccCond, &graphMutex);
        }

        if (graphHasLargeSCC && !prevLargeSCC) {
            cout << "At Least 50% of the graph belongs to the same SCC\n";
        } else if (!graphHasLargeSCC && prevLargeSCC) {
            cout << "At Least 50% of the graph no longer belongs to the same SCC\n";
        }
        prevLargeSCC = graphHasLargeSCC;
        pthread_mutex_unlock(&graphMutex);
    }
    return nullptr;
}

// Function to accept new connections
void acceptConnections(int serverSocket) {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            cerr << "Failed to accept connection.\n";
            continue;
        }

        cout << "New client connected: " << clientSocket << endl;

        // Launch a thread for the new client
        thread([clientSocket]() {
            unordered_map<int, pair<int, int>> pendingEdges; // Map to track edges for "Newgraph"
            char buffer[1024];

            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

                if (bytesRead <= 0) {
                    // Client disconnected
                    cout << "Client disconnected: " << clientSocket << endl;
                    close(clientSocket);
                    break;
                }

                string command(buffer);
                string response = processCommand(clientSocket, command, pendingEdges);

                // Send response to the client
                if (write(clientSocket, response.c_str(), response.size()) == -1) {
                    cerr << "Error sending response to client.\n";
                    break;
                }
            }
        }).detach();
    }
}

// Main function to start the server
int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket.\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Failed to bind socket.\n";
        return 1;
    }

    if (listen(serverSocket, 5) == -1) {
        cerr << "Failed to listen on socket.\n";
        return 1;
    }

    cout << "Server is running on port 8080...\n";

    // Launch a thread to monitor SCC size
    pthread_t monitorThread;
    pthread_create(&monitorThread, nullptr, sccMonitor, nullptr);

    // Accept connections in the main thread
    acceptConnections(serverSocket);

    close(serverSocket);
    return 0;
}
