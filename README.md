# C++ Project Suite

This repository is a collection of C++ projects that explore different systems programming concepts, including agent-based systems, state machines, network programming, and RPC mechanisms.

## Projects

### `agent_chef_problem`
A C++ simulation of the classic agent-chef synchronization problem, demonstrating concurrency, synchronization primitives (e.g., mutexes, condition variables), and multi-threaded design.

### `state_machine`
An implementation of a finite state machine (FSM) framework in C++. This project showcases clean design patterns for modeling systems with discrete states and transitions.

### `UDP_client_host_server`
A simple UDP-based client-server system written in C++. Includes examples of:
- Socket programming
- Sending and receiving datagrams
- Basic networking concepts

### `UDP_based_RPC`
A minimal Remote Procedure Call (RPC) framework built over UDP. Demonstrates:
- Serializing and deserializing data
- Defining remote procedures
- Handling requests and responses in a distributed system

---

## 🛠 Build & Run

Each project has its own folder with source code.  
To build a specific project (for example, `agent_chef_problem`):

```bash
cd agent_chef_problem
g++ -std=c++17 -o agent_chef main.cpp  # adjust filenames as needed
./agent_chef
