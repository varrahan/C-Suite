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
 * A RPC server that processes requests and sends to host.
 */
class Server : private Socket {
private:
    bool invalid_flag = false; // flag to terminate program when true
    struct sockaddr_in hostAddr; // Host address information
    /**
     * Processes incoming UDP requests.
     * @param packet The received packet.
     * @return The response packet to send back to the client. 
     */
   std::vector<uint8_t> processRequest(const std::vector<uint8_t>& packet) {
        // Validate the received packet
        if(!Datagram::isValidRequest(packet)) {
            std::cerr << "Invalid packet format" << std::endl;
            invalid_flag = true;
            return {0, 5, 'i', 'n', 'v', 'a', 'l', 'i', 'd'};
        }
        // Prepare response based on request type
        std::vector<uint8_t> response;
        if(packet[1] == 1) {  // Read request
            response = Datagram::createDataOrAck(true, {'d', 'a', 't', 'a'});
        } else {  // Write request
            response = Datagram::createDataOrAck(false, {'a', 'c', 'k'});
        }
        return response;
    }

    /**
     * Sends a request to the intermediate host to request data from the client.
     * @return True if the request was sent successfully, false otherwise.
     */
    bool sendRequest() {
        std::vector<uint8_t> requestPacket = {0, 9}; // arbitrary request number
        std::cout << "Server sending request for data to host:" << std::endl;
        Datagram::printPacket(requestPacket);
        // Send request to host
        if (!rpcSend(requestPacket, hostAddr)) {
            std::cerr << "Failed to send request to host" << std::endl;
            return false;
        }
        // Wait for response from host
        std::vector<uint8_t> clientRequest;
        if (!rpcReply(clientRequest)) {
            std::cerr << "No response from host" << std::endl;
            return false;
        }
        std::cout << "Received request from client to host:" << std::endl;
        Datagram::printPacket(clientRequest);
        // Process request and send back to host
        std::vector<uint8_t> response = processRequest(clientRequest);
        std::cout << "Sending response back to host:" << std::endl;
        Datagram::printPacket(response);
        if (!rpcSend(response, hostAddr)) {
            std::cerr << "Failed to send response to host" << std::endl;
            return false;
        }
        // Wait for ack from host
        std::vector<uint8_t> ack;
        if (!rpcReply(ack)) {
            std::cerr << "No acknowledgment from host" << std::endl;
            return false;
        }
        std::cout << "Received acknowledgment from host:" << std::endl;
        Datagram::printPacket(ack);
        return true;
    }  
public:
    /**
     * Constructs a Server instance and binds it to port 50069. 
     * Initializes the server and binds it to a non-privileged port for communication.
     */
    Server() {
        bind(50069);  // Non-privileged port
        hostAddr.sin_family = AF_INET;
        hostAddr.sin_port = htons(50024);
        hostAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        std::cout << "Server initialized on port 50069" << std::endl;
    }

    /**
     * Runs the server in an loop until invalid flag is rasied or 11 cycles are completed.
     */
    void run() {
        std::cout << "Server running" << std::endl;
        int count = 0;
        while(true) {
            std::cout << "\nRequest cycle #" << (count + 1) << std::endl;
            if(invalid_flag || count >= 11) {
                std::cerr << "Invalid packet received. Terminating server" << std::endl;
                return;
            }
            if (!sendRequest()) {
                std::cerr << "Failed to complete RPC cycle" << std::endl;
            }
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
};

/**
 * Initializes and runs the server.
 */
int main() {
    try {
        Server server;
        server.run();
    } catch(const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
