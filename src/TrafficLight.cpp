#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> ulock(_mqMutex);
    
    // Use condition variable to wait until the _queue is NOT empty anymore (message arrived!)
    // Used lambda function with this = MessageQueue instance
    // Also pass ulock to condition_variable. (wait() is releasing the lock, so that the state of queue can be checked.)
    _cond.wait(ulock, [this]{ return !_msgs.empty();} );

    T msg = std::move(_msgs.back());
    _msgs.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> ulock(_mqMutex);

    //std::cout << "Traffic light message (" << msg << ") has been sent!" << std::endl;
    _msgs.clear(); // it's more efficient to add this line for the center intersection that gets used alot.
    _msgs.emplace_back(std::move(msg));
    _cond.notify_one(); // Notify after pushing a new message to the queue

}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

TrafficLight::~TrafficLight(){}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true){
        TrafficLightPhase msg = _queue.receive();
        if(msg == TrafficLightPhase::green){
            break;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method ???cycleThroughPhases??? should be started in a thread when the public method ???simulate??? is called. 
    // To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // Get random int between 4000 and 6000
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<int> distr(4000, 6000);

    int cycle_duration = distr(eng);

    // Get current time to measure elapsed time
    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed;

    while(true){
        // Measure elapsed time in seconds
        elapsed = std::chrono::high_resolution_clock::now() - start;

        if (elapsed.count()*1000 > cycle_duration){
            // Reset time variables
            cycle_duration = distr(eng);
            start = std::chrono::high_resolution_clock::now();

            // Toggle _currentPhase
            if(_currentPhase == TrafficLightPhase::red){
                _currentPhase = TrafficLightPhase::green;
            }
            else{
                _currentPhase = TrafficLightPhase::red;
            }
            
            // send message queue using move semantics
            TrafficLightPhase msg = _currentPhase;
            _queue.send(std::move(msg));
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

