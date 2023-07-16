#include <iostream>
#include <thread>
#include <string>
#include "ClientSocket.hpp"

void HandleReceive(ClientSocket* mySocket) {
    while(mySocket->IsConnected()) {
        std::cout << mySocket->ReceiveData() << "\n";
    }
}

int main() {
    
    // Starts connection
    std::cout << "To start connection, use the command /connect <server_ip> <server_port>\n";
    std::string serverAddress;
    int serverPort;
    
    std::string auxString = "";
    while(auxString != "/connect") {
        std::cin >> auxString >> serverAddress >> serverPort;    
    }

    ClientSocket mySocket(serverAddress, serverPort);

    if(!mySocket.Connect()) {
        std::cout << "Error during connection...\n";
        return 0;
    }
    
    std::cout << "Connection complete!\n";

    std::cout << "To join a channel, use the command /join <channel_name>\n";
    std::string channelName;
    
    while(auxString != "/join") {
        std::cin >> auxString >> channelName;
    }
    mySocket.SendData(auxString + " " + channelName);

    std::thread ReceiveThread(HandleReceive, &mySocket);

    while(true) {
        std::string s;
        std::getline(std::cin, s);
        mySocket.SendData(s);
        if(s == "/quit")
            break;
    }
    
    mySocket.Disconnect();

    return 0;
}