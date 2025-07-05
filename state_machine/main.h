/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: March 1, 2025
Title: Assignment 3
*/

#ifndef TRAFFIC_LIGHT_SYSTEM_H
#define TRAFFIC_LIGHT_SYSTEM_H

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <ctime>
#include <sstream>
#include <iomanip>

/**
 * Enumeration representing different types of events in the system.
 */
enum class Event { TIMEOUT, PEDESTRIAN_BUTTON };

class State;
class Context;

/**
 * Get the current timestamp as a string.
 * @return A string representing the current timestamp in the format [YYYY-MM-DD HH:MM:SS].
 */
std::string getCurrentTimestamp();

/**
 * Logs a message with a timestamp.
 * @param message The message to be logged.
 */
void logMessage(const std::string& message);

/**
 * Abstract base class representing a state in the traffic light system.
 */
class State : public std::enable_shared_from_this<State> {
public:
    virtual ~State() = default;

    /**
     * Handles the TIMEOUT event.
     * @param context The context of the state machine.
     * @return The next state.
     */
    virtual std::shared_ptr<State> timeout(Context* context) = 0;

    /**
     * Handles the PEDESTRIAN_BUTTON event.
     * @param context The context of the state machine.
     * @return The next state.
     */
    virtual std::shared_ptr<State> pedestrianWaiting(Context* context) = 0;

    /**
     * Entry action for the state.
     * @param context The context of the state machine.
     */
    virtual void entry(Context* context) = 0;

    /**
     * Exit action for the state.
     * @param context The context of the state machine.
     */
    virtual void exit(Context* context) = 0;

    /**
     * Get the name of the state.
     * @return The name of the state as a string.
     */
    virtual std::string getName() const = 0;
};

/**
 * Context class representing the state machine and traffic light system.
 */
class Context {
private:
    std::shared_ptr<State> currentState;
    std::atomic<bool> isPedestrianWaiting;
    mutable std::mutex mtx;
    std::atomic<bool> running;
    std::queue<Event> eventQueue;
    std::condition_variable cv;
    std::thread eventThread;

public:
    /**
     * Constructs a Context object and starts event processing.
     */
    Context();

    /**
     * Destructor to stop event processing and clean up resources.
     */
    ~Context();

    /**
     * Sets the current state.
     * @param state The new state to transition to.
     */
    void setState(std::shared_ptr<State> state);

    /**
     * Queues an event for processing.
     * @param event The event to be queued.
     */
    void queueEvent(Event event);

    /**
     * Processes events from the event queue.
     */
    void processEvents();

    /**
     * Triggers a timeout event.
     */
    void timeout();

    /**
     * Triggers a pedestrian waiting event.
     */
    void pedestrianWaiting();

    /**
     * Sets the pedestrian waiting flag.
     * @param value Boolean indicating if a pedestrian is waiting.
     */
    void setIsPedestrianWaiting(bool value);

    /**
     * Gets the pedestrian waiting flag.
     * @return True if a pedestrian is waiting, false otherwise.
     */
    bool getIsPedestrianWaiting() const;

    /**
     * Signals vehicles with a specified signal.
     * @param signal The signal to be displayed to vehicles.
     */
    void signalVehicles(const std::string& signal);

    /**
     * Signals pedestrians with a specified signal.
     * @param signal The signal to be displayed to pedestrians.
     */
    void signalPedestrians(const std::string& signal);

    /**
     * Starts a timer that triggers a timeout event after a specified number of seconds.
     * @param seconds The duration in seconds for the timer.
     */
    void startTimer(int seconds);

    /**
     * Get the name of the current state.
     * @return The name of the current state as a string, or "NO_STATE" if no state is set.
     */
    std::string getCurrentStateName() const;
};

/**
 * State representing the vehicles green light phase.
 */
class VehiclesGreen : public State {
public:
    std::string getName() const override;
    void entry(Context* context) override;
    void exit(Context*) override;
    std::shared_ptr<State> timeout(Context* context) override;
    std::shared_ptr<State> pedestrianWaiting(Context*) override;
};

/**
 * State representing the vehicles yellow light phase.
 */
class VehiclesYellow : public State {
public:
    std::string getName() const override;
    void entry(Context* context) override;
    void exit(Context*) override;
    std::shared_ptr<State> timeout(Context*) override;
    std::shared_ptr<State> pedestrianWaiting(Context*) override;
};

/**
 * State representing the pedestrians walk signal phase.
 */
class PedestriansWalk : public State {
public:
    std::string getName() const override;
    void entry(Context* context) override;
    void exit(Context*) override;
    std::shared_ptr<State> timeout(Context*) override;
    std::shared_ptr<State> pedestrianWaiting(Context*) override;
};

/**
 * State representing the pedestrians flashing signal phase.
 */
class PedestriansFlash : public State {
private:
    int flashCounter;
    void handleFlash(Context* context);

public:
    /**
     * Constructs a PedestriansFlash state object.
     */
    PedestriansFlash();

    std::string getName() const override;
    void entry(Context* context) override;
    void exit(Context*) override;
    std::shared_ptr<State> timeout(Context* context) override;
    std::shared_ptr<State> pedestrianWaiting(Context*) override;
};

#endif