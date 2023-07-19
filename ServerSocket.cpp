#include "ServerSocket.hpp"

ServerSocket::ServerSocket(int serverPort) {
    sockServerAddress.sin_family = AF_INET;
    sockServerAddress.sin_addr.s_addr = INADDR_ANY;
    sockServerAddress.sin_port = htons(serverPort);
    
    socketId = socket(AF_INET, SOCK_STREAM, 0);
}

bool ServerSocket::Bind() {
    if(socketId < 0)
        return false;

    int opt = 1;
    
    // Forcefully attaching socket to the port
    if(setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        return false;

    if(bind(socketId, (struct sockaddr*)&sockServerAddress, sizeof(sockServerAddress)) < 0) {
        return false;
    }

    listen(socketId, 5);
    return true;
}

int ServerSocket::Accept() {
    struct sockaddr_in sockClientAddress;
    socklen_t clilen = sizeof(sockClientAddress);
    int socketClientId = accept(socketId, (struct sockaddr*)&sockClientAddress, &clilen);
    clientAdresses[socketClientId] = sockClientAddress;
    return socketClientId;
}

bool ServerSocket::SendData(const std::string& data, int clientId) {
    writeSocketMutex[clientId].lock();
    const char* dataPtr = data.c_str();
    int totalSizeToSend = data.size();
    
    while(totalSizeToSend > 0) {
        int sizeToSend = std::min(maxSize, totalSizeToSend);
        int bytesSent = send(clientId, dataPtr, sizeToSend, 0);
        if(bytesSent <= 0) {
            writeSocketMutex[clientId].unlock();
            return false;
        }
        totalSizeToSend -= bytesSent;
        dataPtr += bytesSent;
    }
    
    writeSocketMutex[clientId].unlock();
    return true;
}

std::string ServerSocket::ReceiveData(int clientId) {
    char buffer[maxSize];
    memset(buffer, 0, maxSize);
    
    int bytesRead = recv(clientId, buffer, maxSize - 1, 0);
    return bytesRead > 0 ? std::string(buffer) : std::string("");
}

// Returns IP as string by client ID
std::string ServerSocket::GetIPBySocketID(int clientId) {
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &(clientAdresses[clientId].sin_addr), str, INET_ADDRSTRLEN );

    return std::string(str);
}
