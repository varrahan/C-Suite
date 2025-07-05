#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <atomic>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>

/**
 * @class Datagram
 * Provides utility functions for creating and handling datagrams.
 */
class Datagram {
public:
    /**
     * Creates a Datagram packet.
     * @param filename The name of the file to read.
     * @param mode The mode of transfer.
     * @param isRead The request type, true if read request false if write request
     * @return A vector containing the read reqeust packet data.
     */
    static std::vector<uint8_t> createRequest(const std::string& filename, const std::string& mode, bool isRead) {
        std::vector<uint8_t> packet = {0, static_cast<uint8_t>(isRead ? 1 : 2)};
        packet.insert(packet.end(), filename.begin(), filename.end());
        packet.push_back(0);  // Zero byte after filename
        packet.insert(packet.end(), mode.begin(), mode.end());
        packet.push_back(0);  // Zero byte after mode
        return packet;
    }

    /**
     * Creates a Data or Ack packet.
     * @param isData True if the packet is a data packet, false ack packet.
     * @param data The data to include in the packet.
     * @return A vector containing the data or ack packet data.
     */
    static std::vector<uint8_t> createDataOrAck(bool isData, const std::vector<uint8_t>& data) {
        if (isData) {
            // Create data packet
            std::vector<uint8_t> packet = {0, 3};
            packet.push_back((1 >> 8) & 0xFF);
            packet.push_back(1 & 0xFF);
            packet.insert(packet.end(), data.begin(), data.end());
            return packet;
        } else {
            // Create ack packet
            std::vector<uint8_t> packet = {0, 4};
            packet.push_back((0 >> 8) & 0xFF);
            packet.push_back(0 & 0xFF);
            return packet;
        }
    }

    /**
     * Prints the packet as both raw bytes and a human-readable string.
     * @param packet The packet to print.
     */
    static void printPacket(const std::vector<uint8_t>& packet) {
        std::cout << "Packet as bytes: ";
        for(uint8_t byte : packet) {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << "\nPacket as string: ";
        for(uint8_t byte : packet) {
            if(isprint(byte)) {
                std::cout << static_cast<char>(byte);
            } else {
                std::cout << "[" << static_cast<int>(byte) << "]";
            }
        }
        std::cout << std::endl;
    }
    
    /**
     * Validates whether the given packet follows the expected format of a request.
     * @param packet The packet to validate.
     * @return True if the packet is a valid request, false otherwise.
     */
    static bool isValidRequest(const std::vector<uint8_t>& packet) {
        if(packet.size() < 4) return false;
        if(packet[0] != 0) return false;
        if(packet[1] != 1 && packet[1] != 2) return false;
        // Find first zero after filename
        size_t firstZero = 2;
        while(firstZero < packet.size() && packet[firstZero] != 0) firstZero++;
        if(firstZero >= packet.size()) return false;
        // Find second zero after mode
        size_t secondZero = firstZero + 1;
        while(secondZero < packet.size() && packet[secondZero] != 0) secondZero++;
        if(secondZero >= packet.size()) return false;
        // Ensure nothing after second zero
        return secondZero == packet.size() - 1;
    }
};

/**
 * A base class for handling UDP socket communication.
 */
class Socket {
protected:
    int sockfd; // The socket file descriptor
    struct sockaddr_in addr; // The socket address

    /**
     * Constructs a Socket and initializes the socket..
     */
    Socket() : sockfd(-1) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0) {
            perror("Socket creation failed");
            throw std::runtime_error("Error creating socket");
        } 
        // Enable address reuse
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            perror("setsockopt failed");
            throw std::runtime_error("Error setting socket option");
        }
    }

    /**
     * Socket destructor
     */
    ~Socket() {
        if(sockfd >= 0) {
            close(sockfd);
        }
    }

public:
    /**
     * Binds the socket to the specified port.
     * @param port The port number to bind to.
     * @throws std::runtime_error if binding fails.
     */
    void bind(uint16_t port) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        // Check if socket was socket bind was successful
        if(::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Bind failed");
            throw std::runtime_error("Error binding socket");
        }
    }

    /**
     * Receives a packet from the socket.
     * @param packet The received packet.
     * @return True if a packet was received, false otherwise.
     */
    bool rpcReply(std::vector<uint8_t>& packet){
        char buf[1024];
        struct sockaddr_in resAddr;
        socklen_t resAddrLen = sizeof(resAddr);
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity == 0) {  // Timeout occurred
            std::cerr << "Timeout: No response received within 5 seconds" << std::endl;
            return false;
        } else if (activity < 0) {
            perror("Error during select()");
            return false;
        }
        int n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&resAddr, &resAddrLen);
        if (n < 0) {
            perror("Error receiving response");
            return false;
        }
        packet.assign(buf, buf + n);
        return true;
    }

    /**
     * Sends packet to address and waits for response.
     * @param packet The packet to send
     * @param addr The address to send the packet to
     * @return True if the packet was sent and a response was received else false
     */
    bool rpcSend(const std::vector<uint8_t>& packet, const struct sockaddr_in& addr) {
        ssize_t sent = sendto(sockfd, packet.data(), packet.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
        if(sent < 0) {
            perror("Send failed");
            return false;
        }
        return true; 
    }
};
#endif // DATAGRAM_H