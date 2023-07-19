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

// These global variable have multiple purposes, and are used by different threads

// Mutexes
std::mutex queueMutex; // Locks access to the message queue
std::mutex clientIdsMutex; // Locks who access clientIds
std::mutex nickToIdMutex; // Locks who access nickToId
std::map<std::string, std::mutex> channelToClientsMutex; // Locks access to the set of client of each channel
std::map<std::string, std::mutex> channelMutedIdsMutex; // Locks access to the muted clients of each channel

// Message queue
std::queue<std::pair<std::string,std::string>> messageQueue;

// Information about clients and IDs
std::set<int> clientIds;
std::map<std::string, int> nickToId;
std::map<std::string, std::set<int>> channelToClients;
std::map<std::string, std::set<int>> channelMutedIds;

#define MAX_RETRIES 5 //numero maximo de retries para enviar a mensagem

//trata o sinal do ctrl + C para ser ignorado e mandar a mensagem
void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Signal SIGINT received." << std::endl;
    }
}

// Checks (Thread Safe) if id is still a connected client
bool isConnected(int id) {
    clientIdsMutex.lock();
    bool value = clientIds.count(id) == 1;
    clientIdsMutex.unlock();

    return value;
}

// This function handles the receiving of messages from a given client
void HandleClient(ServerSocket* mySocket, int id) {
    bool isAdmin = false; // Local variable that says if client is admin at current channel
    bool isInChannel = false; // Local variable that says if client is currently in a channel

    std::cout << "Entrando " << id << "\n";
    std::vector<std::string> tokens;
    std::istringstream iss;
    
    std::string nickname = "Unknown" + std::to_string(id);
    nickToIdMutex.lock();
    nickToId[nickname] = id;
    nickToIdMutex.unlock();
    bool setNickname = false;
    std::string channel;
    
    // This is the main loop of this function
    // While the connection persists, the loop continues.
    while(isConnected(id)) {
        std::string s = mySocket->ReceiveData(id);
        if(!isConnected(id))
            break;

        // Filtering empty data
        if(s == "")
            continue;

        // Splitting the string in tokens
        iss = std::istringstream(s);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(tokens));
        
        // Handling each possible command
        
        // /quit or EOF ends connection
        if(tokens[0] == "/quit") {
            mySocket->SendData("quit", id); // Thread Safe
            break;
        }
        
        // /join allows the user to join a channel
        else if(tokens[0] == "/join") {
            // If the channel already exists
            if(channelToClients.count(tokens[1]) != 0) {
                // If the client is already in a channel, removes participation from old channel
                if(isInChannel) {
                    isAdmin = false;
                    channelToClientsMutex[channel].lock();
                    channelToClients[channel].erase(id); 
                    channelToClientsMutex[channel].unlock();
                }
                
                isInChannel = true;
                channel = tokens[1];
                channelToClientsMutex[channel].lock();
                channelToClients[channel].insert(id);
                channelToClientsMutex[channel].unlock();
            }

            // If the channel doesn't exist yet
            else {
                // If the client is already in a channel, removes participation from old channel
                if(isInChannel) {
                    channelToClientsMutex[channel].lock();
                    channelToClients[channel].erase(id);
                    channelToClientsMutex[channel].unlock();
                }
                
                isInChannel = true;
                isAdmin = true;
                channel = tokens[1];
                channelToClients[channel] = std::set<int>();
                channelToClients[channel].insert(id);
                channelMutedIds[channel] = std::set<int>();
            }

            tokens.clear();
            continue;
        }

        // /ping tests the connection sending pong back 
        else if(tokens[0] == "/ping") {
            queueMutex.lock();
            mySocket->SendData("pong", id); // Thread Safe
            queueMutex.unlock();
            tokens.clear();
            continue;
        }
        
        // /nickname allows nickname changing
        else if(tokens[0] == "/nickname") {
            nickToIdMutex.lock();
            if(nickToId.count(tokens[1]) != 0) {
                nickToIdMutex.unlock();
                break;
            }
            nickToId[tokens[1]] = id;
            nickname = tokens[1];
            setNickname = true;
            tokens.clear();
            nickToIdMutex.unlock();
            continue;
        }
        
        // /whois allows admins to get the IP address of a chosen user
        else if(tokens[0] == "/whois") {
            nickToIdMutex.lock();
            // If user isn't connected to a channel
            if(!isInChannel) {
                mySocket->SendData("This command can only be executed after joining a channel", id); // Thread Safe
            }
            
            // If user has admin permission
            else if(isAdmin) {
                if(nickToId.count(tokens[1]) == 1) {
                    std::string userIP = mySocket->GetIPBySocketID(nickToId[tokens[1]]);
                    mySocket->SendData(tokens[1] + " IP: " +  userIP, id); // Thread Safe
                }
                else {
                    mySocket->SendData("User " + tokens[1] + " not found", id); // Thread Safe
                }
            }
            
            // If user doesn't have admin permission
            else {
                mySocket->SendData(std::string("You do not have permission to use /whois"), id); // Thread Safe
            }

            tokens.clear();
            nickToIdMutex.unlock();
            continue;
        }

        // /mute allows admins to mute a chosen user
        else if(tokens[0] == "/mute") {
            nickToIdMutex.lock();
            channelMutedIdsMutex[channel].lock();
            channelToClientsMutex[channel].lock();
            // If user isn't connected to a channel
            if(!isInChannel) {
                mySocket->SendData("This command can only be executed after joining a channel", id); // Thread Safe
            }
            
            // If user has admin permission
            else if(isAdmin) {
                if(channelMutedIds[channel].count(nickToId[tokens[1]]) == 0 and channelToClients[channel].count(nickToId[tokens[1]]) != 0)
                    channelMutedIds[channel].insert(nickToId[tokens[1]]);

            }
            
            // If user doesn't have admin permission
            else {
                mySocket->SendData(std::string("You do not have permission to use /mute"), id); // Thread Safe
            }

            tokens.clear();
            nickToIdMutex.unlock();
            channelMutedIdsMutex[channel].unlock();
            channelToClientsMutex[channel].unlock();
            continue;
        }

        // /unmute allows admin to revert the muting of a chosen user
        else if(tokens[0] == "/unmute") {
            nickToIdMutex.lock();
            channelMutedIdsMutex[channel].lock();
            channelToClientsMutex[channel].lock();
            // If user isn't connected to a channel
            if(!isInChannel) {
                mySocket->SendData("This command can only be executed after joining a channel", id); // Thread Safe
            }
            
            // If user has admin permission
            else if(isAdmin) {
                if(channelMutedIds[channel].count(nickToId[tokens[1]]) != 0 and channelToClients[channel].count(nickToId[tokens[1]]) != 0)
                    channelMutedIds[channel].erase(nickToId[tokens[1]]);
            }
            
            // If user doesn't have admin permission
            else {
                mySocket->SendData(std::string("You do not have permission to use /unmute"), id); // Thread Safe
            }

            tokens.clear();
            nickToIdMutex.unlock();
            channelMutedIdsMutex[channel].unlock();
            channelToClientsMutex[channel].unlock();
            continue;
        }
        
        // /kick kicks a chosen user
        else if(tokens[0] == "/kick") {
            nickToIdMutex.lock();
            channelToClientsMutex[channel].lock();

            // If user isn't connected to a channel
            if(!isInChannel) {
                mySocket->SendData("This command can only be executed after joining a channel", id); // Thread Safe
            }

            // If user has admin permission
            else if(isAdmin) {
                clientIdsMutex.lock();
                if(channelToClients[channel].count(nickToId[tokens[1]]) != 0) {
                    clientIds.erase(nickToId[tokens[1]]);
                    channelToClients[channel].erase(nickToId[tokens[1]]);
                    mySocket->SendData("quit", id);
                }

                clientIdsMutex.unlock();
            }
            
            // If user doesn't have admin permission
            else {
                mySocket->SendData(std::string("You do not have permission to use /kick"), id); // Thread Safe
            }

            tokens.clear();
            nickToIdMutex.unlock();
            channelToClientsMutex[channel].unlock();
            continue;
        }
        
        // Error message if client is not in channel
        if(!isInChannel) {
            mySocket->SendData("Join a channel to send messages", id);
        }

        // If it isn't a command and the user isn't muted, the message is put in the message queue
        channelMutedIdsMutex[channel].lock();
        else if(channelMutedIds[channel].count(id) == 0){
            queueMutex.lock();
            messageQueue.push(std::make_pair(channel, nickname + ": " + s));
            queueMutex.unlock();
        }
        channelMutedIdsMutex[channel].unlock();
        tokens.clear();
    }


    // Closing client connection and ajusting related variables
    // Erasing id from nickToId mapping
    nickToIdMutex.lock();
    if(setNickname)
        nickToId.erase(nickname);
    nickToIdMutex.unlock();

    // Erasing id from channel
    channelToClientsMutex[channel].lock();
    if(channelToClients[channel].count(id) != 0)
        channelToClients[channel].erase(id);
    channelToClientsMutex[channel].unlock();

    // Erasing id from set of connected clients, if not erased before
    if(isConnected(id)) {
        clientIdsMutex.lock();
        clientIds.erase(id);
        clientIdsMutex.unlock();
    }

    // Closing the socket
    close(id);

    std::cout << "Fechou o " << id << "\n";
}

// This function handles the sending of messages to all the clients in a channel
void PrintMessages(ServerSocket* mySocket) {
    while(true) {
        queueMutex.lock();
        if(!messageQueue.empty()) {
            for (auto &id: channelToClients[messageQueue.front().first]) {
                for (int i = 0; i < MAX_RETRIES; i++) {
                    if (mySocket->SendData(messageQueue.front().second, id)) { // Thread Safe
                        break;
                    }
                    else if (i >= MAX_RETRIES - 1) {
                        clientIdsMutex.lock();
                        clientIds.erase(id);
                        clientIdsMutex.unlock();
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
        clientIds.insert(newId);
        queueMutex.unlock();
    }

    return 0;
}
