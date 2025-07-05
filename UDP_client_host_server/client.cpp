/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: February 8, 2025
Title: Assignment 2
*/
#include "datagram.h"

/**
 * RPC client that communicates with a server using an intermediate host service.
 */
class Client : private Socket {
private:
    struct sockaddr_in serverAddr;  // Server address structure 
    std::string filename;           // Filename for requests 

    /**
     * Sends a read or write request to the server based on the request number. 
     * Request types alternate between read and write, except for the 10th request, which is invalid.
     * @param requestNum The request number to determine request type.
     */
    void sendRequest(int requestNum) {
        const std::string mode = "netascii";
        
        std::vector<uint8_t> packet;
        if (requestNum == 10) {
            packet = {0, 5, 'i', 'n', 'v', 'a', 'l', 'i', 'd'};
        } else if (requestNum % 2 == 0) {
            packet = Datagram::createRequest(filename, mode, true);
        } else {
            packet = Datagram::createRequest(filename, mode, false);
        }
        
        std::cout << "\nSending request #" << (requestNum + 1) << std::endl;
        Datagram::printPacket(packet);
        // Send request to server and receive response
        bool success = rpcSend(packet, serverAddr);
        if (!success) {
            std::cerr << "Error sending request" << std::endl;
            return;
        }
        std::vector<uint8_t> response;
        // Check response
        if(rpcReply(response)) {
            std::cout << "Received response:" << std::endl;
            Datagram::printPacket(response);
        } else {
            std::cerr << "Error receiving response" << std::endl;
        }     
    }

public:
    /**
     * Constructs a Client object and initializes the server address.
     * @param filename The name of the file to be requested from the server.
     */
    Client(std::string filename) : filename(filename) {
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50023);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        std::cout << "Client initialized" << std::endl;
    }

    /**
     * Runs the client by sending multiple requests and receiving responses.
     */
    void run() {
        for (int i = 0; i < 11; i++) {
            sendRequest(i);
        }
    }
};

/**
 * Main function to start the UDP client.
 * @param argc Argument count.
 * @param argv Argument vector, expecting a filename as an argument.
 * @return Exit status code.
 */
int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <filename.txt>" << std::endl;
            return 1;
        }
        // Get filename
        std::string filename = argv[1];
        Client client(filename);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
