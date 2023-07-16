#include <iostream>
#include <thread>
#include <string>
#include "ClientSocket.hpp"
#include <signal.h>

void HandleReceive(ClientSocket* mySocket) {
    while(mySocket->IsConnected()) {
        std::string s = mySocket->ReceiveData();
        std::cout << mySocket->ReceiveData() << "\n";
    }
}

void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal SIGINT received." << std::endl;
    }
}

int main() {
    signal(SIGINT, handleSignal);
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
        std::cout << "VOU MANDAR " << s << "\n";
        mySocket.SendData(s);
        if(s == "/quit")
            break;
    }
    
    mySocket.Disconnect();

    return 0;
}
