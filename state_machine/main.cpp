/* 
Author: Varrahan Uthayan 
Student Number: 101229572
Professor: Dr. Gregory Franks
Course: SYSC3303B
Lab Section: SYSC3303B1
Due Date: March 1, 2025
Title: Assignment 3
*/

#include "main.h"

/**
 * Get the current timestamp as a string.
 * @return std::string representing the current timestamp in the format "YYYY-MM-DD HH:MM:SS".
 */
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * Log a message with a timestamp.
 * @param message The message to log.
 */
void logMessage(const std::string& message) {
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << "[" << getCurrentTimestamp() << "] " << message << std::endl;
}

/**
 * @class Context
 * Represents the context for the state machine.
 *
 * The context maintains the current state and handles events and transitions.
 */
Context::Context() : isPedestrianWaiting(false), running(true) {
    eventThread = std::thread(&Context::processEvents, this);
}

/**
 * Destructor for the Context class. Stops the event processing thread.
 */
Context::~Context() {
    running = false;
    cv.notify_all();
    if (eventThread.joinable()) {
        eventThread.join();
    }
}

/**
 * Set the current state of the state machine.
 * @param state The new state.
 */
void Context::setState(std::shared_ptr<State> state) {
    std::lock_guard<std::mutex> lock(mtx);
    if (currentState) {
        logMessage("STATE TRANSITION: " + currentState->getName() + " -> " + state->getName());
        currentState->exit(this);
    } else {
        logMessage("INITIAL STATE: " + state->getName());
    }
    currentState = state;
    currentState->entry(this);
}

/**
 * Queue an event for processing.
 * @param event The event to queue.
 */
void Context::queueEvent(Event event) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        eventQueue.push(event);
    }
    cv.notify_one();
}

/**
 * Process events from the event queue.
 */
void Context::processEvents() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !eventQueue.empty() || !running; });

        if (!running) break;

        if (!eventQueue.empty()) {
            Event event = eventQueue.front();
            eventQueue.pop();
            lock.unlock();

            if (currentState) {
                std::shared_ptr<State> newState;
                switch (event) {
                    case Event::TIMEOUT:
                        logMessage("PROCESSING: Timer expiry event");
                        newState = currentState->timeout(this);
                        break;
                    case Event::PEDESTRIAN_BUTTON:
                        isPedestrianWaiting = true;
                        logMessage("PROCESSING: Pedestrian button event");
                        newState = currentState->pedestrianWaiting(this);
                        break;
                }
                if (newState && newState != currentState) {
                    setState(newState);
                }
            }
        }
    }
}

/**
 * Queue a TIMEOUT event.
 */
void Context::timeout() {
    queueEvent(Event::TIMEOUT);
}

/**
 * Queue a PEDESTRIAN_BUTTON event.
 */
void Context::pedestrianWaiting() {
    queueEvent(Event::PEDESTRIAN_BUTTON);
}

/**
 * Set the pedestrian waiting flag.
 * @param value True if a pedestrian is waiting, false otherwise.
 */
void Context::setIsPedestrianWaiting(bool value) {
    isPedestrianWaiting = value;
}

/**
 * Get the pedestrian waiting flag.
 * @return True if a pedestrian is waiting, false otherwise.
 */
bool Context::getIsPedestrianWaiting() const {
    return isPedestrianWaiting;
}

/**
 * Signal vehicles with a given signal.
 * @param signal The signal to display to vehicles.
 */
void Context::signalVehicles(const std::string& signal) {
    logMessage("SIGNAL -> Vehicles: " + signal);
}

/**
 * Signal pedestrians with a given signal.
 * @param signal The signal to display to pedestrians.
 */
void Context::signalPedestrians(const std::string& signal) {
    logMessage("SIGNAL -> Pedestrians: " + signal);
}

/**
 * Start a timer that triggers a TIMEOUT event after a given number of seconds.
 * @param seconds The duration of the timer.
 */
void Context::startTimer(int seconds) {
    std::thread([this, seconds]() {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        timeout();
    }).detach();
}

/**
 * Get the name of the current state.
 * @return The name of the current state as a string, or "NO_STATE" if no state is set.
 */
std::string Context::getCurrentStateName() const {
    std::lock_guard<std::mutex> lock(mtx);
    return currentState ? currentState->getName() : "NO_STATE";
}

/**
 * Returns the name of the state as a string.
 * @return The name of the state "VehiclesGreen".
 */
std::string VehiclesGreen::getName() const {
    return "VehiclesGreen";
}

/**
 * Entry action for the VehiclesGreen state. Signals the traffic lights and starts a timer.
 * @param context Pointer to the Context object managing the state.
 */
void VehiclesGreen::entry(Context* context) {
    context->signalVehicles("GREEN");
    context->signalPedestrians("DONT_WALK");
    context->startTimer(10);
}

/**
 * Exit action for the VehiclesGreen state.
 * @param context Pointer to the Context object managing the state.
 */
void VehiclesGreen::exit(Context*) {}

/**
 * Handles timeout event for VehiclesGreen state.
 * @param context Pointer to the Context object managing the state.
 * @return The next state depending on whether a pedestrian is waiting.
 */
std::shared_ptr<State> VehiclesGreen::timeout(Context* context) {
    if (context->getIsPedestrianWaiting()) {
        return std::shared_ptr<State>(new VehiclesYellow());
    }
    context->startTimer(10);
    return shared_from_this();
}

/**
 * Handles pedestrian waiting event. Does nothing and stays in the same state.
 * @param context Pointer to the Context object managing the state.
 * @return Shared pointer to the current state.
 */
std::shared_ptr<State> VehiclesGreen::pedestrianWaiting(Context*) {
    return shared_from_this();
}

// VehiclesYellow Implementation

/**
 * Returns the name of the state as a string.
 * @return The name of the state "VehiclesYellow".
 */
std::string VehiclesYellow::getName() const {
    return "VehiclesYellow";
}

/**
 * Entry action for the VehiclesYellow state. Signals the traffic lights and starts a timer.
 * @param context Pointer to the Context object managing the state.
 */
void VehiclesYellow::entry(Context* context) {
    context->signalVehicles("YELLOW");
    context->signalPedestrians("DONT_WALK");
    context->startTimer(3);
}

/**
 * Exit action for the VehiclesYellow state. Currently does nothing.
 * @param context Pointer to the Context object managing the state.
 */
void VehiclesYellow::exit(Context*) {}

/**
 * Handles timeout event for VehiclesYellow state.
 * @param context Pointer to the Context object managing the state.
 * @return The next state (PedestriansWalk).
 */
std::shared_ptr<State> VehiclesYellow::timeout(Context*) {
    return std::shared_ptr<State>(new PedestriansWalk());
}

/**
 * Handles pedestrian waiting event. Does nothing and stays in the same state.
 * @param context Pointer to the Context object managing the state.
 * @return Shared pointer to the current state.
 */
std::shared_ptr<State> VehiclesYellow::pedestrianWaiting(Context*) {
    return shared_from_this();
}

// PedestriansWalk Implementation

/**
 * Returns the name of the state as a string.
 * @return The name of the state "PedestriansWalk".
 */
std::string PedestriansWalk::getName() const {
    return "PedestriansWalk";
}

/**
 * Entry action for the PedestriansWalk state. Signals the traffic lights, resets pedestrian waiting flag, and starts a timer.
 * @param context Pointer to the Context object managing the state.
 */
void PedestriansWalk::entry(Context* context) {
    context->signalVehicles("RED");
    context->signalPedestrians("WALK");
    context->setIsPedestrianWaiting(false);
    context->startTimer(15);
}

/**
 * Exit action for the PedestriansWalk state. Currently does nothing.
 * @param context Pointer to the Context object managing the state.
 */
void PedestriansWalk::exit(Context*) {}

/**
 * Handles timeout event for PedestriansWalk state.
 * @param context Pointer to the Context object managing the state.
 * @return The next state (PedestriansFlash).
 */
std::shared_ptr<State> PedestriansWalk::timeout(Context*) {
    return std::shared_ptr<State>(new PedestriansFlash());
}

/**
 * Handles pedestrian waiting event. Does nothing and stays in the same state.
 * @param context Pointer to the Context object managing the state.
 * @return Shared pointer to the current state.
 */
std::shared_ptr<State> PedestriansWalk::pedestrianWaiting(Context*) {
    return shared_from_this();
}

// PedestriansFlash Implementation

/**
 * Constructor for PedestriansFlash state. Initializes the flash counter.
 */
PedestriansFlash::PedestriansFlash() : flashCounter(7) {}

/**
 * Returns the name of the state as a string.
 * @return The name of the state "PedestriansFlash".
 */
std::string PedestriansFlash::getName() const {
    return "PedestriansFlash";
}

/**
 * Entry action for the PedestriansFlash state. Starts flashing pedestrian signal and starts a timer.
 * @param context Pointer to the Context object managing the state.
 */
void PedestriansFlash::entry(Context* context) {
    flashCounter = 7;
    handleFlash(context);
    context->startTimer(1);
}

/**
 * Exit action for the PedestriansFlash state. Currently does nothing.
 * @param context Pointer to the Context object managing the state.
 */
void PedestriansFlash::exit(Context*) {}

/**
 * Handles timeout event for PedestriansFlash state.
 * @param context Pointer to the Context object managing the state.
 * @return The next state depending on the flash counter.
 */
std::shared_ptr<State> PedestriansFlash::timeout(Context* context) {
    flashCounter--;

    if (flashCounter == 0) {
        return std::shared_ptr<State>(new VehiclesGreen());
    }

    handleFlash(context);
    context->startTimer(1);
    return shared_from_this();
}

/**
 * Handles pedestrian waiting event. Does nothing and stays in the same state.
 * @param context Pointer to the Context object managing the state.
 * @return Shared pointer to the current state.
 */
std::shared_ptr<State> PedestriansFlash::pedestrianWaiting(Context*) {
    return shared_from_this();
}

/**
 * Handles flashing behavior for pedestrian signals.
 * @param context Pointer to the Context object managing the state.
 */
void PedestriansFlash::handleFlash(Context* context) {
    if ((flashCounter & 1) == 0) {
        context->signalPedestrians("DONT_WALK");
    } else {
        context->signalPedestrians("BLANK");
    }
}