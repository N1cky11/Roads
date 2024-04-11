#include <cstddef>
#include <thread>
#include <semaphore>
#include <mutex>
#include <chrono>
#include <functional>
#include <iostream>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <queue>


class Car {
public:
    size_t carID;
    size_t timeToPass;
    size_t direction;
};

class TrafficLight {
private:
    std::atomic<bool> pass1;
    std::mutex m;
    std::condition_variable cv;
    std::jthread lightThread;
    size_t timeToSwitch;

public:
    TrafficLight(size_t time);
    size_t getTimeToSwitch() const;
    void switchLight();
    void requestStop();
    bool canPass(int direction);
    bool waitToPass(int direction);
};


class Road {
private:
    TrafficLight m_light;
    std::mutex cout_mutex;
    std::queue<Car> notPassedCars{};
    std::counting_semaphore<1> direction1{1};
    std::counting_semaphore<2> direction2{1};
    std::counting_semaphore<1> direction3{1};
    std::counting_semaphore<2> direction4{1};

public:
    static size_t totalCarChecked;
    static size_t totalCars;

    Road(size_t timeToSwitchLight);
    ~Road();

    void carPassing(const Car &obj);
};


class RoadSimulation {
private:
    std::vector<Car> m_cars;
    Road m_road1;
    Road m_road2;
    std::vector<std::jthread> threads;

public:
    RoadSimulation(std::vector<size_t>& timeArrival, std::vector<size_t>& cars, std::vector<size_t>& carsDirection);
    void operator()();
};
