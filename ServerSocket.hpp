#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <mutex>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class ServerSocket {

private:
    int socketId;
    static constexpr int maxSize = 4096; // Max message size according to specification
    struct sockaddr_in sockServerAddress;
    std::map<int, std::mutex> writeSocketMutex;
    std::map<int, struct sockaddr_in> clientAdresses;
public:
    // Default Constructor
    ServerSocket(int port);
    
    // Binds socket
    // Returns true if success and false otherwise
    bool Bind();

    // Accepts new connection
    // Returns the socket file descriptor of the new client
    int Accept();

    // Sends data up to max limit
    // Returns true if success and false otherwise
    bool SendData(const std::string& data, int clientId);

    // Receives data up to max limit
    // Returns true if success and false otherwise
    std::string ReceiveData(int clientId);

    // Returns IP as string by client ID
    std::string GetIPBySocketID(int clientId);
};
