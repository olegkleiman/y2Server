//
//  main.cpp
//  y2Server
//
//  Created by Oleg Kleiman on 25/08/2022.
//

#include <iostream>
#include <algorithm>
#include <sys/types.h>

// headers for socket(), getaddrinfo() and friends
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

int MAX_CLIENTS = 5;
int MAX_MSG_LENGTH = 256;

int main(int argc, const char * argv[]) {
    
    if( argc < 2 ) {
        cerr << "Usage: " << argv[0] << "port=NUM" << endl;
        return  1;
    }
    
    cout << argv[1] << std::endl;

    std::string delimiter = "=";
    std::string s = argv[1];
    size_t pos  = s.find(delimiter);
    if( pos == string::npos ) {
        cout << "Define post number, i.e. -port=NNN" << endl;
        return 1;
    }
    string strPortNum = s.substr(pos+delimiter.length());
    int portNum = stoi(strPortNum);
    
    addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;  // SOCK_STREAM refers to TCP
    hints.ai_flags    = AI_PASSIVE;
    
    int gAddRes = getaddrinfo(NULL, strPortNum.c_str(), &hints, &res);
    if (gAddRes != 0) {
        std::cerr << gai_strerror(gAddRes) << "\n";
        return -2;
    }
    
    char ipStr[INET6_ADDRSTRLEN];
    while( res != NULL ) {
        
        void *addr;
        std::string ipVer;
        
        // if address is ipv4 address
        if( res->ai_family == AF_INET ) {
            ipVer             = "IPv4";
            sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(res->ai_addr);
            addr              = &(ipv4->sin_addr);
        } else {
            ipVer              = "IPv6";
            sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(res->ai_addr);
            addr               = &(ipv6->sin6_addr);
        }
        inet_ntop(res->ai_family, addr, ipStr, sizeof(ipStr));
        cout << ipVer << " : " << ipStr << endl;
        
        res = res->ai_next;
    }
    
    int server_fd = socket(AF_INET, // domain: the protocol family of socket being requested
                           SOCK_STREAM, // type: the type of socket within that family
                           0 // and the protocol
                           );
    if( server_fd < 0 ) {
        cout << "Can't create a server socket" << endl;
        return  -1;
    }
    int opt = 1;
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    
    address.sin_family = AF_INET;
    // automatically be filled with current host's IP address
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( portNum );
    
    // associate that socket with a port on our local machine
    if (::bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        cerr << "Can't bind s server socket" << endl;
        return -1;
    }
    
    // This listen() call tells the socket to listen to the incoming connections.
    // The listen() function places all incoming connection into a backlog queue
    // until accept() call accepts the connection.
    // Here, we set the maximum size for the backlog queue to MAX_CLIENTS.
    if( listen(server_fd, MAX_CLIENTS) == -1 ) {
        freeaddrinfo(res);
        cerr << "Error while Listening on socket" << endl;
        return  -1;
    }
    
    cout << "[Server] Listening on port " << portNum << endl;
    
    //Accept a client
    struct sockaddr_in client_addr;
    socklen_t len = sizeof client_addr;
    
    int remote_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t*)&len);
    if( remote_socket < 0 ) {
        cerr << "Error on accept" << endl;
        return -1;
    }
    
    printf("server: got connection from %s port %d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    cout << "connected" << std::endl;
    
    char szBuffer[MAX_MSG_LENGTH];
    
    while( 1) {

        ssize_t recv_len = recv(remote_socket, szBuffer, MAX_MSG_LENGTH, 0);
        if( recv_len <= 0 ) {
            if (recv_len == 0) {
                cout << "Client disconnected" << endl;
                return 1;
            }
        }
        
        cout << "Received: " << szBuffer << endl;
        
        /* Clearing the  buffer with 0's for the next reply */
        memset(&szBuffer[0], 0, sizeof(szBuffer));
    }
    
    return 0;
}
