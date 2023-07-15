#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class ClientSocket {

private:
    int socketId;
    bool connected;
    struct sockaddr_in sockServerAddress;
    static constexpr int maxSize = 4096; // Max message size according to specification
    
public:
    // Default Constructor
    ClientSocket(std::string serverAddress, int serverPort);
    
    // Connects to sockServerAddres
    // Returns true if success and false otherwise
    bool Connect();

    // Ends current connection
    // Returns true if success and false otherwise
    bool Disconnect();
    
    // Checks if connection is active
    bool IsConnected();

    // Sends data up to max limit
    // Returns true if success and false otherwise
    bool SendData(const std::string& data);

    // Receive data up to max limit
    // Returns true if success and false otherwise
    std::string ReceiveData();
};
