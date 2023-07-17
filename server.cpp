#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <mutex>
#include "ServerSocket.hpp"
#include <queue>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <signal.h>

std::mutex queueMutex;
std::queue<std::pair<std::string,std::string>> messageQueue;
std::vector<int> clientIds;
std::map<std::string, int> nickToId;
std::map<std::string, std::set<int>> channelToClients;
std::map<std::string, std::string> channelToAdmin;

#define MAX_RETRIES 5

void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal SIGINT received." << std::endl;
    }
}

void HandleClient(ServerSocket* mySocket, int id) {
    bool isAdmin;
    std::cout << "Entrando " << id << "\n";
    std::string joinMessage = mySocket->ReceiveData(id);
    std::cout << "Recebi o join " << joinMessage << "\n";
    std::vector<std::string> tokens;
    std::istringstream iss(joinMessage);
    std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));
    
    std::string channel;
    if(tokens[0] != "/join") {
        close(id);
        return;
    }

    if(channelToClients.count(tokens[1]) != 0) {
        queueMutex.lock();
        channelToClients[tokens[1]].insert(id);
        queueMutex.unlock();
        std::cout << "Inseri no canal\n";
        isAdmin = false;
    }
    
    else {
        isAdmin = true;
        queueMutex.lock();
        channelToClients[tokens[1]] = std::set<int>();
        channelToClients[tokens[1]].insert(id);
        std::cout << "Criou canal " << tokens[1] << "\n";
        queueMutex.unlock();
    }
    
    channel = tokens[1];
    tokens.clear();

    std::string nickname = "Unknown " + std::to_string(id);
    bool setNickname = false;
    while(std::find(clientIds.begin(), clientIds.end(),id) != clientIds.end()) {
        std::string s = mySocket->ReceiveData(id);
        if(s == "")
            continue;
        iss = std::istringstream(s);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));
        
        if(tokens[0] == "/quit") {
            break;
        }
        else if(tokens[0] == "/ping") {
            queueMutex.lock();
            mySocket->SendData("pong", id);
            queueMutex.unlock();
            tokens.clear();
            continue;
        }
        
        else if(tokens[0] == "/nickname") {
            if(nickToId.count(tokens[1]) != 0) {
                break;
            }
            nickToId[tokens[1]] = id;
            nickname = tokens[1];
            setNickname = true;
            tokens.clear();
            continue;
        }
        
        else if(tokens[0] == "/whois") {
            if(isAdmin) {
                if(nickToId.count(tokens[1]) == 1) {
                    std::string userIP = mySocket->GetIPBySocketID(nickToId[tokens[1]]);
                    mySocket->SendData(tokens[1] + " IP: " +  userIP, id);
                }
                else {
                    mySocket->SendData("User " + tokens[1] + " not found", id); 
                }
            }
            else {
                mySocket->SendData(std::string("You do not have permission to use /whois"), id);
            }
            tokens.clear();
            continue;
        }

        queueMutex.lock();
        messageQueue.push(std::make_pair(channel, nickname + ": " + s));
        queueMutex.unlock();
        tokens.clear();
    }
    
    queueMutex.lock();
    if(setNickname)
        nickToId.erase(nickname);
    
    channelToClients[channel].erase(id);

    for(int i = 0; i < clientIds.size(); ++i) {
        if(clientIds[i] == id) {
            clientIds.erase(clientIds.begin() + i);
            break;
        }
    }
    queueMutex.unlock();
    close(id);
    std::cout << "Fechou o " << id << "\n";
}

void PrintMessages(ServerSocket* mySocket) {
    while(true) {
        queueMutex.lock();
        if(!messageQueue.empty()) {
            for (auto &id: channelToClients[messageQueue.front().first]) {
                for (int i = 0; i < MAX_RETRIES; i++) {
                    if (mySocket->SendData(messageQueue.front().second, id)) {
                        std::cout << "Enviou para o " << id << "\n";
                        break;
                    }
                    else {
                        std::cout << "Falhou envio para o " << id << " " << i << " vez\n";
                        if (i >= MAX_RETRIES - 1)
                            clientIds.erase(std::find(clientIds.begin(), clientIds.end(),id));
                    }
                }
            }
            messageQueue.pop();
        }
        queueMutex.unlock();
    }
}

int main() {
    int port;

    signal(SIGINT, handleSignal);

    std::cout << "Port: ";
    std::cin >> port;

    ServerSocket mySocket(port);
    
    mySocket.Bind();
    std::vector<std::thread> clientThreads;
    std::thread printThread(PrintMessages, &mySocket);

    while(true) {
        int newId = mySocket.Accept();
        queueMutex.lock();
        clientThreads.push_back(std::thread(HandleClient, &mySocket, newId));
        clientIds.push_back(newId);
        queueMutex.unlock();
    }

    return 0;
}
