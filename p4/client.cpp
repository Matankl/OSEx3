#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9037);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    string input;
    while (true) {
        cout << "Enter command: ";
        getline(cin, input);
        send(sock, input.c_str(), input.size(), 0);

        if (input == "Exit") {
            break;
        }
        read(sock, buffer, 1024);
        cout << "Server response: " << buffer << endl;
    }

    close(sock);
    return 0;
}
