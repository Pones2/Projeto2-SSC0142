#include "ClientSocket.hpp"

ClientSocket::ClientSocket(std::string serverAddress, int serverPort) {
    sockServerAddress.sin_family = AF_INET;
    sockServerAddress.sin_addr.s_addr = inet_addr(serverAddress.c_str());
    sockServerAddress.sin_port = htons(serverPort);
    
    socketId = socket(AF_INET, SOCK_STREAM, 0);
    connected = false;
}

bool ClientSocket::Connect() {
    if(connected or socketId < 0)
        return false;

    connect(socketId, (const sockaddr*)&sockServerAddress, sizeof(sockServerAddress));
    
    connected = true;
    return true;
}

bool ClientSocket::Disconnect() {
    if(!connected)
        return false;

    close(socketId);
    connected = false;

    return true;
}

bool ClientSocket::SendData(const std::string& data) {
    const char* dataPtr = data.c_str();
    int totalSizeToSend = data.size();
    
    while(totalSizeToSend > 0) {
        int sizeToSend = std::min(maxSize, totalSizeToSend);
        int bytesSent = send(socketId, dataPtr, sizeToSend, 0);
        if(bytesSent == -1)
            return false;
        totalSizeToSend -= bytesSent;
        dataPtr += bytesSent;
    }
    
    return true;
}

std::string ClientSocket::ReceiveData() {
    char buffer[maxSize];
    memset(buffer, 0, maxSize);

    int bytesRead = recv(socketId, buffer, maxSize - 1, 0);

    return bytesRead > 0 ? std::string(buffer) : std::string("");
}

bool ClientSocket::IsConnected() {
    return connected;
}
