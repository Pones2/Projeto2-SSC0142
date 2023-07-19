#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include "ClientSocket.hpp"
#include <signal.h>


// Handles received data
void HandleReceive(ClientSocket* mySocket) {
    while(mySocket->IsConnected()) {
        std::string s = mySocket->ReceiveData();
        if(s != "" and s != "quit")
            std::cout << s << "\n";
        else if (s == "quit") {
            std::cout << "You were disconnected\n";
            mySocket->Disconnect();
        }
    }
}

// Handles the SIGINT Signal, (CTRL+C).
void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal SIGINT received." << std::endl;
    }
}

// Checks if nickname is in correct format
bool isNicknameValid(std::string s) {
    constexpr int maxLength = 50;
    if(s.length() > maxLength)
        return false;

    return true;
}

// Checks if channel name is in correct format
bool isChannelNameValid(std::string s) {
    constexpr int maxLength = 200;
    if(s.length() > maxLength)
        return false;
    
    if(s[0] != '#' and s[0] != '&')
        return false;

    for(int i = 0; i < (int) s.length(); ++i) {
        if(s[i] == 7 or s[i] == ',')
            return false;
    }

    return true;
}

int main() {
    signal(SIGINT, handleSignal);
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

    std::thread ReceiveThread(HandleReceive, &mySocket);

    std::vector<std::string> tokens;
    std::istringstream iss;

    // Program main loop
    // While connected, keeps sending messages
    while(mySocket.IsConnected()) {
        std::string s;
        std::getline(std::cin, s);
        iss = std::istringstream(s);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));

        // Sends message and quits if necessary
        if(s == "/quit" or std::cin.eof()) {
            mySocket.SendData("/quit");
            break;
        }
        
        else if(tokens.size() == 0) {
            tokens.clear();
            continue;
        }

        // Ignores invalid channel names
        else if(tokens[0] == "/join") {
            if(isChannelNameValid(tokens[1]) and (int)tokens.size() == 2)
                mySocket.SendData(s);
        }

        // Ignores invalid nicknames
        else if(tokens[0] == "/nickname") {
            if(isNicknameValid(tokens[1]) and (int)tokens.size() == 2)
                mySocket.SendData(s);
        }

        else
            mySocket.SendData(s);

        tokens.clear();
    }
    
    // Ends the connection with the server
    mySocket.Disconnect();
    ReceiveThread.join();

    return 0;
}
