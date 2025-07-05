/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: March 1, 2025
Title: Assignment 3
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include "main.h"

/**
 * Test function to simulate a normal traffic light cycle with pedestrian interaction.
 * 
 * This test verifies that the state machine transitions as expected when a pedestrian presses
 * the button while the vehicle light is green.
 * 
 * @param context The context object representing the current state of the traffic light system.
 */
void test_normal_cycle(Context& context) {
    std::cout << "\n=== Testing Normal Cycle with Pedestrian ===\n";

    context.setState(std::shared_ptr<State>(new VehiclesGreen()));
    assert(context.getCurrentStateName() == "VehiclesGreen");

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "\nSimulating pedestrian button press\n";
    context.pedestrianWaiting();

    std::this_thread::sleep_for(std::chrono::seconds(35));
    assert(context.getCurrentStateName() == "VehiclesGreen");
}

/**
 * Test function to simulate multiple button presses by pedestrians.
 * 
 * This test simulates multiple pedestrian button presses while the vehicle light is green,
 * ensuring that each press is handled correctly within the cycle.
 * 
 * @param context The context object representing the current state of the traffic light system.
 */
void test_multiple_button_presses(Context& context) {
    std::cout << "\n=== Testing Multiple Button Presses ===\n";

    context.setState(std::shared_ptr<State>(new VehiclesGreen()));
    assert(context.getCurrentStateName() == "VehiclesGreen");

    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "\nSimulating pedestrian button press #" << (i + 1) << "\n";
        context.pedestrianWaiting();
    }

    std::this_thread::sleep_for(std::chrono::seconds(35));
    assert(context.getCurrentStateName() == "VehiclesGreen");
}

/**
 * Test function to simulate a scenario where no pedestrian presses the button.
 * 
 * This test ensures that the traffic light remains green without any changes when no pedestrian
 * presses the button.
 * 
 * @param context The context object representing the current state of the traffic light system.
 */
void test_no_pedestrian(Context& context) {
    std::cout << "\n=== Testing No Pedestrian Scenario ===\n";

    context.setState(std::shared_ptr<State>(new VehiclesGreen()));
    assert(context.getCurrentStateName() == "VehiclesGreen");

    std::this_thread::sleep_for(std::chrono::seconds(15));
    assert(context.getCurrentStateName() == "VehiclesGreen");
}

/**
 * Test function to simulate pressing the pedestrian button during a WALK signal.
 * 
 * This test checks if the system properly handles a pedestrian button press while the pedestrian
 * light is already in the "WALK" state.
 * 
 * @param context The context object representing the current state of the traffic light system.
 */
void test_button_during_pedestrian_walk(Context& context) {
    std::cout << "\n=== Testing Button Press During Walk Signal ===\n";

    context.setState(std::shared_ptr<State>(new VehiclesGreen()));
    assert(context.getCurrentStateName() == "VehiclesGreen");

    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "\nSimulating first pedestrian button press\n";
    context.pedestrianWaiting();

    std::this_thread::sleep_for(std::chrono::seconds(8));
    std::cout << "\nSimulating second pedestrian button press during WALK\n";
    context.pedestrianWaiting();

    std::this_thread::sleep_for(std::chrono::seconds(25));
    assert(context.getCurrentStateName() == "VehiclesGreen");
}

/**
 * Main function to execute the traffic light state machine tests.
 * 
 * This function runs a series of tests to verify the correct behavior of the traffic light system
 * under various scenarios, including normal cycles, multiple button presses, and edge cases.
 * 
 * @return 0 if all tests pass successfully.
 */
int main() {
    std::cout << "Starting Traffic Light State Machine Tests\n";
    std::cout << "==========================================\n";

    {
        Context context;
        test_normal_cycle(context);
    }
    std::cout << "\nPress Enter to continue to next test...";
    std::cin.get();

    {
        Context context;
        test_multiple_button_presses(context);
    }
    std::cout << "\nPress Enter to continue to next test...";
    std::cin.get();

    {
        Context context;
        test_button_during_pedestrian_walk(context);
    }
    std::cout << "\nPress Enter to continue to next test...";
    std::cin.get();

    {
        Context context;
        test_no_pedestrian(context);
    }
    std::cout << "\nAll tests completed successfully!\n";
    return 0;
}
