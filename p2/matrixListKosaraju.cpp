#include <iostream>
#include <list>
#include <vector>

using namespace std;

class Graph {
    int n;  // Number of vertices
    vector<vector<int>> adjMatrix;  // Adjacency matrix

public:
    // Constructor to initialize the graph with 'n' vertices
    Graph(int vertices) : n(vertices) {
        adjMatrix.resize(n + 1, vector<int>(n + 1, 0));  // Initialize an n+1 x n+1 matrix with 0s
    }

    // Function to add an edge from vertex 'u' to vertex 'v'
    void addEdge(int u, int v) {
        adjMatrix[u][v] = 1;  // Set the matrix entry to 1 for edge u -> v
    }

    // Get the number of vertices
    int getNumVertices() const {
        return n;
    }

    // Get the adjacency matrix of the graph
    const vector<int>& getAdjMatrix(int v) const {
        return adjMatrix[v];
    }

    // Function to create and return the transposed graph (reverse edges)
    Graph transposeGraph() const {
        Graph transposed(n);  // Create a new graph with the same number of vertices

        // Reverse all edges from the original graph
        for (int u = 1; u <= n; ++u) {
            for (int v = 1; v <= n; ++v) {
                if (adjMatrix[u][v] == 1) {
                    transposed.addEdge(v, u);  // Reverse edge u -> v becomes v -> u
                }
            }
        }

        return transposed;
    }
};

// Helper function to perform DFS and fill the list with vertices in order of completion time
void fillOrder(const Graph &g, int v, vector<bool> &visited, list<int> &Stack) {
    visited[v] = true;  // Mark the current vertex as visited

    // Recur for all adjacent vertices in the original graph
    const vector<int>& adjRow = g.getAdjMatrix(v);
    for (int i = 1; i <= g.getNumVertices(); ++i) {
        if (adjRow[i] == 1 && !visited[i]) {
            fillOrder(g, i, visited, Stack);
        }
    }

    // Push the current vertex to the list after visiting all its neighbors
    Stack.push_back(v);
}

// A DFS function to explore all vertices in the reversed graph
void dfs(const Graph &g, int v, vector<bool> &visited, list<int> &component) {
    visited[v] = true;   // Mark the current vertex as visited
    component.push_back(v);  // Add the current vertex to the current component

    // Recur for all adjacent vertices in the reversed graph
    const vector<int>& adjRow = g.getAdjMatrix(v);
    for (int i = 1; i <= g.getNumVertices(); ++i) {
        if (adjRow[i] == 1 && !visited[i]) {
            dfs(g, i, visited, component);
        }
    }
}

// Function to print the strongly connected components (SCCs) using Kosaraju's algorithm
void printSCCs(const Graph &g) {
    int n = g.getNumVertices();
    list<int> Stack;
    vector<bool> visited(n + 1, false);  // Initialize visited array for the first DFS

    // Step 1: Perform DFS on the original graph to fill the list
    for (int i = 1; i <= n; ++i) {
        if (!visited[i]) {
            fillOrder(g, i, visited, Stack);
        }
    }

    // Step 2: Get the transposed graph
    Graph transposed = g.transposeGraph();

    // Step 3: Reset the visited array for the second DFS
    fill(visited.begin(), visited.end(), false);

    // Step 4: Process vertices in order of decreasing finishing time (from list)
    while (!Stack.empty()) {
        int v = Stack.back();
        Stack.pop_back();

        // If this vertex hasn't been visited, it's part of a new SCC
        if (!visited[v]) {
            list<int> component;  // Stores the current SCC
            dfs(transposed, v, visited, component);  // Perform DFS on reversed graph for this SCC

            // Print the current strongly connected component
            for (int vertex : component) {
                cout << vertex << " ";
            }
            cout << endl;  // Newline after each SCC
        }
    }
}

int main() {
    int n, m;
    
    // Input: Read the number of vertices (n) and edges (m)
    cin >> n >> m;

    Graph g(n);  // Create a graph with 'n' vertices

    // Input: Read the 'm' edges
    for (int i = 0; i < m; ++i) {
        int u, v;
        cin >> u >> v;  // Read edge from vertex u to vertex v
        g.addEdge(u, v);  // Add the edge to the graph
    }

    // Output: Print the strongly connected components (SCCs)
    printSCCs(g);

    return 0;
}
