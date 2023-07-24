#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define PORT 777


#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <map>

using namespace std;

class CommandParser
{
private:
    std::map<int, std::string> commandBuff = {};

public:
    std::string getCommand(int socketid);
    void addData(int socketid, const char* data, int length);
    void clear(int socketid);
};

//A generic way to handle Network Connections
class ConnectionHandler
{
public:
    virtual ~ConnectionHandler() = default;

    /**
     * @return true if we should read more commands, else false
     */
    virtual bool handle(int socketid) = 0;

    bool finished = false;
};

class SimpleCommandHandler : public ConnectionHandler
{
public:
    SimpleCommandHandler(std::size_t readBufferSize, CommandParser& commandParser)
        : buffLen(readBufferSize),
        parser(commandParser)
    {}
    SimpleCommandHandler(const SimpleCommandHandler&) = delete;
    bool handle(int socketid) override;

private:
    std::string readLine(int socketid);
    void sendResult(int socketid, std::string result);
    const std::size_t buffLen;
    CommandParser& parser;
};

#include <string>

//Dummy Value to be changed
#define MAXPENDING 5

class Exception
{
public:
    Exception(std::string message)
        : message(message)
    {}
    std::string getMessage();
private:
    std::string message;
};

#include <cstring>
#include <exception>
#include <stdexcept>

class NetworkException : public runtime_error {
public:
    NetworkException(std::string message)
        : runtime_error(message + ": " + std::strerror(errno))
    {}
};

class TCPServer
{
public:
    TCPServer(int port, const std::string&, ConnectionHandler& connection);
    ~TCPServer();
    void tcplisten();
private:
    //Socket file Descriptor
    int socket;
    ConnectionHandler& connection;
};


//int main()
//{
//    WSADATA wsaData;
//    int iResult;
//
//    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
//    if (iResult != 0) {
//        printf("WSAStartup failed: %d\n", iResult);
//        return 1;
//    }
//    CommandParser p;
//    SimpleCommandHandler connection(10, p);
//    TCPServer server(777, "192.168.0.5", connection);
//    server.tcplisten();
//}



#include <cstring>
#include <iostream>
#include <string>

std::string Exception::getMessage() {
    return message;
}

TCPServer::TCPServer(int port, const std::string& address, ConnectionHandler& connection)
    : socket(::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)),
    connection(connection)
{
    if (socket < 0) {
        throw NetworkException("SOCKET Error: could not create basic socket");
    }

    struct sockaddr_in s_address; // server address
    memset(&s_address, 0, sizeof s_address);
    s_address.sin_family = AF_INET;
    s_address.sin_addr.s_addr = ::inet_addr(address.c_str());
    s_address.sin_port = ::htons(port);

    if (bind(socket, (struct sockaddr*)&s_address, sizeof s_address) < 0) {
        throw NetworkException("SOCKET Error: Failed to bind a socket");
    }

    if (listen(socket, MAXPENDING) < 0) {
        throw NetworkException("SOCKET Error: Failed to listen");
    }

}

void TCPServer::tcplisten()
{
    while (!connection.finished) {
        struct sockaddr_in address;     /* Client address */
        socklen_t length = sizeof address;
        int client;                    /* Socket descriptor for client */
        if ((client = ::accept(socket, (struct sockaddr*)&address, &length)) < 0) {
            std::cerr << "Failed to accept: " << std::strerror(errno) << std::endl;
            return;
        }
        std::clog << "Handling client: " << ::inet_ntoa(address.sin_addr) << std::endl;

        static auto constexpr message = "WELCOME\n";
        send(client, message, std::strlen(message), 0);

        while (connection.handle(client));
        closesocket(client);

        // discard remaining received commands
    }
}

TCPServer::~TCPServer()
{
    ::closesocket(socket);
}


#include <cstring>
#include <iostream>
#include <string>
#include <vector>

std::string SimpleCommandHandler::readLine(int socketid)
{
    std::vector<char> storage(buffLen);
    char* const buffer = storage.data();

    int recvSize = 0;
    while ((recvSize = ::recv(socketid, buffer, buffLen - 1, 0)) > 0) {
        parser.addData(socketid, buffer, recvSize);
    }

    return parser.getCommand(socketid);
}

void SimpleCommandHandler::sendResult(int socketid, std::string result) {
    send(socketid, result.c_str(), result.length() + 1, 0);
}

bool SimpleCommandHandler::handle(int socketid)
{
    std::string command = readLine(socketid);
    std::clog << "Command received: " << command << std::endl;

    if (command == "exit") {
        sendResult(socketid, "Thank You Very Much.\nBye.\n");
        parser.clear(socketid);
        return false;
    }
    else if (command == "stop") {
        sendResult(socketid, "Server exiting.\n");
        parser.clear(socketid);
        finished = true;
        return false;
    }
    else {
        sendResult(socketid, "Ignoring command '" + command + "'.\n");
        return true;
    }
}

#include <string>

void CommandParser::addData(int socketid, const char* data, int length) {
    commandBuff[socketid].append(data, length);
}

std::string CommandParser::getCommand(int socketid)
{
    std::string& buffer = commandBuff[socketid];

    std::size_t pos = buffer.find('\n');
    if (pos == std::string::npos) {
        return "";
    }

    std::string fetchedCommand = buffer.substr(0, pos);
    buffer = buffer.substr(pos + 1);
    return fetchedCommand;
}

void CommandParser::clear(int socketid)
{
    commandBuff[socketid].clear();
}