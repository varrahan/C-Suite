/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: January 25, 2025
Title: Assignment 1
*/
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

/**
 * A Monitor class that provides synchronized access to a shared data structure using mutexes and condition variables.
 */
class Monitor {
private:
    mutex mtx; // Mutex for synchronizing access to shared data.
    condition_variable cv; // Condition variable for synchronizing threads.
    /**
     * Struct containing shared data, an exit flag, and an access name to indicate which consumer thread can access the mutex.
     */
    struct DATA_BUS {
        vector<string> shared_data; // Vector to store shared data used by threads.
        string access_name; // Name of the consumer thread allowed to access the mutex.
        bool exit_flag = false; // Exit flag to end all threads once set to true.
    } bus;

public:
    /**
     * Adds items to the shared data structure and assigns access to a specific chef.
     * @param ingredients A vector of ingredients to be added.
     * @param assignedChef The name of the chef assigned to access the shared data.
     */
    void addItem(vector<string>& ingredients, string assignedChef) {
        unique_lock<mutex> lock(mtx); // Lock the mutex.
        cv.wait(lock, [this]() { return bus.shared_data.empty() && bus.access_name == ""; }); ///< Wait until shared data is empty.
        bus.access_name = assignedChef; // Assign the specific chef to shared data.
        for (string& ingredient : ingredients) {
            bus.shared_data.push_back(ingredient); // Add data.
        }
        cv.notify_all(); // Notify all threads.
    }
    /**
     * Sets exit flag to true.
     */
    void setExitFlag() {
        unique_lock<mutex> lock(mtx);
        bus.exit_flag = true;
        cv.notify_all(); // Notify all chef threads to exit.
    }

    /**
     * Retrieves items from the shared data structure for a specific chef.
     * @param assignedChef The name of the chef assigned to access the shared data.
     * @return A vector of strings containing the retrieved ingredients.
     */
    vector<string> getItem(string assignedChef) {
        unique_lock<mutex> lock(mtx); // Lock the mutex.
        vector<string> results; // Vector to store results for the chef.

        cv.wait(lock, [this, assignedChef]() { return (bus.shared_data.size() == 2 && bus.access_name == assignedChef) || bus.exit_flag == true; }); ///< Wait for 2 elements in shared data or wait for exit flag.
        if (bus.exit_flag == true) {
            return {};
        }
        for (string data : bus.shared_data) {
            results.push_back(data); // Add data to results.
        }
        bus.shared_data.clear(); // Clear shared data.
        bus.access_name = "";
        cv.notify_all(); // Notify agent and chefs.
        return results;
    }
};

/**
 * Agent class responsible for adding ingredients to the monitor.
 */
class Agent {
private:
    Monitor& monitor; // Reference to the monitor instance.
    vector<string> ingredients; // List of ingredients.
    int runs; // Number of runs to perform.

public:
    /**
     * Constructs an Agent object.
     * @param mon Reference to the monitor instance.
     * @param items List of ingredients.
     * @param runs Number of runs to perform.
     */
    Agent(Monitor& mon, vector<string> items, int runs) : monitor(mon), ingredients(items), runs(runs) {}

    /**
     * Adds ingredients to the monitor in random pairs.
     */
    void addIngredients() {
        random_device rd;
        mt19937 gen(rd()); // Random number generator.

        for (int i = 0; i < runs; i++) {
            vector<string> copy = ingredients;
            vector<string> results;

            for (int i = 2; i > 0; i--) {
                uniform_int_distribution<> dis(0, i);
                int randomIndex = dis(gen); // Generate a random index.

                results.push_back(copy[randomIndex]); // Add ingredient to results.
                copy.erase(copy.begin() + randomIndex); // Remove selected ingredient.
            }

            monitor.addItem(results, copy[0]); ///< Add items to shared data.
            cout << "Agent added " << results[0] << " and " << results[1] << " to shared data" << endl;
            this_thread::sleep_for(chrono::milliseconds(1)); ///< Sleep to prevent jumbled output.
        }
        monitor.setExitFlag(); // Sets exit flag to terminate all chef threads once agent has completed all runs
    }
};

/**
 * Chef class responsible for consuming ingredients from the monitor.
 */
class Chef {
private:
    Monitor& monitor; // Reference to the monitor instance.
    string ingredient; // Ingredient required by the chef.
    mutex mtx; // Mutex for synchronizing access to shared data.
    condition_variable cv; // Condition variable for synchronizing threads.

public:
    static int num_runs; // Number of runs to perform.
    /**
     * Constructs a Chef object.
     * @param mon Reference to the monitor instance.
     * @param item Ingredient required by the chef.
     */
    Chef(Monitor& mon, string item) : monitor(mon), ingredient(item) {}

    /**
     * Removes ingredients from the monitor and consumes them.
     */
    void removeIngredients() {
        while(num_runs > 0) {
            lock_guard<mutex> lock(mtx); // Lock the mutex.
            vector<string> ingredients = monitor.getItem(ingredient); // Retrieve items from shared data.
            if (ingredients.empty()) { // empty ingredients signify that exit flag has been called, so terminate thread
                break;
            }
            cout << "Chef with " << ingredient << " made and ate the sandwich" << endl;
            num_runs--;
            cv.notify_all(); // Notify all threads.
            this_thread::sleep_for(chrono::milliseconds(1)); // Sleep to prevent jumbled output.
        }
    }
};

// Due to Cpp constraits and without a header file, we must initialize static value outside of chef and main
int Chef::num_runs = 1000;

/**
 * Main function to initialize and run the agent and chef threads.
 */
int main() {
    Monitor monitor; // Initialize the monitor.
    vector<string> ingredients = {"bread", "butter", "jam"}; // Ingredients for sandwiches.
    int runs = Chef::num_runs; // Number of runs.

    Agent agent(monitor, ingredients, runs); // Initialize the agent.
    Chef breadChef(monitor, ingredients[0]); // Initialize bread chef.
    Chef butterChef(monitor, ingredients[1]); // Initialize butter chef.
    Chef jamChef(monitor, ingredients[2]); // Initialize jam chef.

    thread agentThread(&Agent::addIngredients, &agent); // Start agent thread.
    thread breadChefThread(&Chef::removeIngredients, &breadChef); // Start bread chef thread.
    thread butterChefThread(&Chef::removeIngredients, &butterChef); // Start butter chef thread.
    thread jamChefThread(&Chef::removeIngredients, &jamChef); // Start jam chef thread.

    agentThread.join(); // Join agent thread.
    breadChefThread.join(); // Join bread chef thread.
    butterChefThread.join(); // Join butter chef thread.
    jamChefThread.join(); // Join jam chef thread.

    return 0;
}