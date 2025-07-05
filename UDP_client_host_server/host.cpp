/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: March 15, 2025
Title: Assignment 4
*/
#include "datagram.h"

/**
 * A UDP-based host that forwards packets between a client and a server.
 */
class Host : private Socket {
    private:
    int clientFd;              // Socket file descripter for client 
    int serverFd;              // Socket file descripter for server
    std::mutex mtx;           // Mutex for queue sync
    std::condition_variable cv; // Condition variable for queue sync
    std::queue<std::vector<uint8_t>> queue; // Queue for packets
    std::atomic<bool> running;       // Flag to run threads
    struct sockaddr_in clientAddr;  // Client address
    struct sockaddr_in serverAddr;      // Server address

    /**
     * Thread to handle client communication.
     */
    void clientHandler() {
        std::cout << "Client handler thread started" << std::endl;
        char buffer[1024];
        while (running) {
            // Wait for client packet
            socklen_t clientAddrLen = sizeof(clientAddr);
            int n = recvfrom(clientFd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
            std::vector<uint8_t> packet(buffer, buffer + n);
            std::cout << "Client handler: Received packet from client:" << std::endl;
            Datagram::printPacket(packet);
            // Add the client packet to queue
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(packet);
            cv.notify_all();
            // Send ack to client
            std::vector<uint8_t> ack = Datagram::createDataOrAck(false, {'a', 'c', 'k'});
            sendto(clientFd, ack.data(), ack.size(), 0, (struct sockaddr*)&clientAddr, clientAddrLen);
            std::cout << "Client handler: Sent acknowledgment to client" << std::endl;
        }
        std::cout << "Client handler thread terminated" << std::endl;
    }

    /**
     * Thread to handle server communication.
     */
    void serverHandler() {
        std::cout << "Server handler thread started" << std::endl;
        char buffer[1024];
        while (running) {
            // Wait for server request
            socklen_t serverAddrLen = sizeof(serverAddr);
            int n = recvfrom(serverFd, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
            std::vector<uint8_t> serverRequest(buffer, buffer + n);
            std::cout << "Server handler: Received request from server:" << std::endl;
            Datagram::printPacket(serverRequest);
            // Check for client data
            std::vector<uint8_t> clientPacket;
            bool hasClientData = false;
            std::unique_lock<std::mutex> lock(mtx);
            if (!queue.empty()) {
                clientPacket = queue.front();
                queue.pop();
                hasClientData = true;
            } else {
                // Wait for client data
                cv.wait_for(lock, std::chrono::seconds(2), [this]{ return !queue.empty(); });
                if (!queue.empty()) {
                    clientPacket = queue.front();
                    queue.pop();
                    hasClientData = true;
                }
            }
            if (hasClientData) {
                // Forward client packet to server
                std::cout << "Server handler: Forwarding client packet to server:" << std::endl;
                Datagram::printPacket(clientPacket);
                sendto(serverFd, clientPacket.data(), clientPacket.size(), 0, (struct sockaddr*)&serverAddr, serverAddrLen);
                // Wait for server response
                n = recvfrom(serverFd, buffer, sizeof(buffer), 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
                std::vector<uint8_t> serverResponse(buffer, buffer + n);
                std::cout << "Server handler: Received response from server:" << std::endl;
                Datagram::printPacket(serverResponse);
                // Send ack to server
                std::vector<uint8_t> ack = Datagram::createDataOrAck(false, {'a', 'c', 'k'});
                sendto(serverFd, ack.data(), ack.size(), 0, (struct sockaddr*)&serverAddr, serverAddrLen);
                // Forward response to client
                sendto(clientFd, serverResponse.data(), serverResponse.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                std::cout << "Server handler: Forwarded response to client" << std::endl;
            } else {
                // Send arbitrary value if no data from client
                std::vector<uint8_t> noData = {0, 0};
                sendto(serverFd, noData.data(), noData.size(), 0, (struct sockaddr*)&serverAddr, serverAddrLen);
                std::cout << "Server handler: No client data available, sent no-data response" << std::endl;
            }
        }
        std::cout << "Server handler thread terminated" << std::endl;
    }

public:
    /**
     * Constructs Host object and initializes sockets and addresses.
     */
    Host() : Socket(), running(true) {
        // Initialize client socket
        clientFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (clientFd < 0) {
            throw std::runtime_error("Failed to create client socket");
        }
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        clientAddr.sin_port = htons(50023);
        if (::bind(clientFd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
            throw std::runtime_error("Failed to bind client socket");
        }
        std::cout << "Client socket initialized on port 50023" << std::endl;
        // Initialize server socket
        serverFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (serverFd < 0) {
            throw std::runtime_error("Failed to create server socket");
        }
        struct sockaddr_in serverSocketAddr;
        memset(&serverSocketAddr, 0, sizeof(serverSocketAddr));
        serverSocketAddr.sin_family = AF_INET;
        serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverSocketAddr.sin_port = htons(50024);
        if (::bind(serverFd, (struct sockaddr*)&serverSocketAddr, sizeof(serverSocketAddr)) < 0) {
            throw std::runtime_error("Failed to bind server socket");
        }
        std::cout << "Server socket initialized on port 50024" << std::endl;
        // Initialize server address
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50069);  // Server port
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        std::cout << "Host initialized" << std::endl;
    }

    /**
     * Host destructor
     */
    ~Host() {
        if (clientFd >= 0) {
            close(clientFd);
        }
        if (serverFd >= 0) {
            close(serverFd);
        }
    }

    /**
     * Runs client and server threads
     */
    void run() {
        std::cout << "Starting host..." << std::endl;
        // Run server and client threads
        std::thread clientThread(&Host::clientHandler, this);
        std::thread serverThread(&Host::serverHandler, this);
        // Stop host after 15 seconds
        std::this_thread::sleep_for(std::chrono::seconds(15));
        running = false;
        // Wait for threads to complete
        clientThread.join();
        serverThread.join();
    }
};

/**
 * Initializes and runs host
 */
int main() {
    try {
        Host host;
        host.run();
    } catch(const std::exception& e) {
        std::cerr << "host error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}