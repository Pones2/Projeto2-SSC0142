#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[MAX_MESSAGE_SIZE] = {0};

    // Criar o descritor de socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar a estrutura de endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Converter o endereço IP do servidor de string para formato binário
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Endereço inválido ou não suportado");
        exit(EXIT_FAILURE);
    }

    // Conectar ao servidor
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao conectar ao servidor");
        exit(EXIT_FAILURE);
    }

    // Loop principal para troca de mensagens
    while (true) {
        // Enviar uma mensagem para o servidor
        std::cout << "Digite uma mensagem para enviar ao servidor (ou 'exit' para sair): ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit")
            break;

        splitAndSend(client_socket, message.c_str());

        // Limpar o buffer antes de receber uma resposta
        memset(buffer, 0, sizeof(buffer));

        // Receber a resposta do servidor
        ssize_t valread = read(client_socket, buffer, sizeof(buffer));
        if (valread < 0) {
            perror("Falha ao receber mensagem");
            break;
        } else if (valread == 0) {
            std::cout << "Conexão encerrada pelo servidor" << std::endl;
            break;
        }

        // Imprimir a resposta recebida do servidor
        std::cout << "Resposta do servidor: " << buffer << std::endl;
    }

    // Fechar o socket do cliente
    close(client_socket);

    return 0;
}
