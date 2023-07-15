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
    socklen_t clilen = sizeof(sockServerAddress);
    return accept(socketId, (struct sockaddr*)&sockServerAddress, &clilen);
}

bool ServerSocket::SendData(const std::string& data, int clientId) {
    const char* dataPtr = data.c_str();
    int totalSizeToSend = data.size();
    
    while(totalSizeToSend > 0) {
        int sizeToSend = std::min(maxSize, totalSizeToSend);
        int bytesSent = send(clientId, dataPtr, sizeToSend, 0);
        if(bytesSent == -1)
            return false;
        totalSizeToSend -= bytesSent;
        dataPtr += bytesSent;
    }
    
    return true;
}

std::string ServerSocket::ReceiveData(int clientId) {
    char buffer[maxSize];
    memset(buffer, 0, maxSize);
    
    int bytesRead = recv(clientId, buffer, maxSize - 1, 0);
    return bytesRead > 0 ? std::string(buffer) : std::string("");
}
