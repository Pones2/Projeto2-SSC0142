#include <iostream>
#include <thread>
#include <string>
#include "ClientSocket.hpp"
#include <signal.h>

void HandleReceive(ClientSocket* mySocket) {//trata as mensagens que recebe do servidor
    while(mySocket->IsConnected()) {
        std::string s = mySocket->ReceiveData();
        if(s != "")
            std::cout << s << "\n";
    }
}

//trata o sinal do ctrl + C para ser ignorado e mandar a mensagem
void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal SIGINT received." << std::endl;
    }
}

int main() {
    signal(SIGINT, handleSignal);
    //comeca a conexao
    std::cout << "To start connection, use the command /connect <server_ip> <server_port>\n";
    std::string serverAddress;
    int serverPort;
    
    std::string auxString = "";
    while(auxString != "/connect") {//espera o comando de conexao
        std::cin >> auxString >> serverAddress >> serverPort;    
    }

    ClientSocket mySocket(serverAddress, serverPort);

    if(!mySocket.Connect()) {//caso a conexao com o socket de errado
        std::cout << "Error during connection...\n";
        return 0;
    }
    
    std::cout << "Connection complete!\n";

    std::cout << "To join a channel, use the command /join <channel_name>\n";
    std::string channelName;
    
    while(auxString != "/join") {//para se conectar com um canal em especifico
        std::cin >> auxString >> channelName;
    }
    mySocket.SendData(auxString + " " + channelName);

    std::thread ReceiveThread(HandleReceive, &mySocket);

    while(true) {//loop de mandar as mensagens para o servidor
        std::string s;
        std::getline(std::cin, s);
        std::cout << "VOU MANDAR " << s << "\n";
        mySocket.SendData(s);
        if(s == "/quit" or std::cin.eof())//espera o ctrl + D ou o quit para parar com o loop
            break;
    }
    
    mySocket.Disconnect();//realiza a desconexao com o servidor

    return 0;
}
