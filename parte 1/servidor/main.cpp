#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

using namespace std;

#define PORT 8080
#define MAX_MESSAGE_SIZE 4096

void splitAndSend(int socket, const char* message) {
    int messageLen = strlen(message);
    int numChunks = (messageLen + MAX_MESSAGE_SIZE - 1) / MAX_MESSAGE_SIZE;
    int sentChunks = 0;

    for (int i = 0; i < numChunks; i++) {
        int chunkSize = std::min(MAX_MESSAGE_SIZE, messageLen - sentChunks * MAX_MESSAGE_SIZE);
        int sentBytes = send(socket, message + sentChunks * MAX_MESSAGE_SIZE, chunkSize, 0);
        if (sentBytes < 0) {
            perror("Falha ao enviar mensagem");
            break;
        }
        sentChunks++;
    }
}

int main() {
    cout << "teste" << endl;
    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[MAX_MESSAGE_SIZE] = {0};

    // Criar o descritor de socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }
    cout << "teste1" << endl;

    // Configurar a estrutura de endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    cout << "teste2" << endl;
    // Vincular o socket a um endereço e porta
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao vincular o socket");
        exit(EXIT_FAILURE);
    }
    cout << "teste3" << endl;

    // Escutar por conexões
    if (listen(server_fd, 3) < 0) {
        perror("Falha ao escutar por conexões");
        exit(EXIT_FAILURE);
    }

    // Aceitar uma nova conexão
    client_len = sizeof(client_addr);
    if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("Falha ao aceitar a conexão");
        exit(EXIT_FAILURE);
    }

    // Loop principal para troca de mensagens
    while (true) {
        // Ler a mensagem recebida do cliente
        ssize_t valread = read(new_socket, buffer, sizeof(buffer));
        if (valread < 0) {
            perror("Falha ao ler mensagem");
            break;
        } else if (valread == 0) {
            std::cout << "Conexão encerrada pelo cliente" << std::endl;
            break;
        }

        // Imprimir a mensagem recebida
        std::cout << "Mensagem recebida: " << buffer << std::endl;

        // Enviar uma mensagem de resposta ao cliente
        std::cout << "Digite uma mensagem para enviar ao cliente (ou 'exit' para sair): ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit")
            break;

        splitAndSend(new_socket, message.c_str());
    }

    // Fechar sockets
    close(new_socket);
    close(server_fd);

    return 0;
}
