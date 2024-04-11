#include "RoadsProblem.h"


    TrafficLight::TrafficLight(size_t time) : pass1(true) , timeToSwitch(time) , lightThread([this] { switchLight(); } )  {} ;

    size_t TrafficLight :: getTimeToSwitch() const {
     return timeToSwitch ;
    }

     void TrafficLight::switchLight() {
        while (!lightThread.get_stop_token().stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(timeToSwitch));
            {
                std::lock_guard<std::mutex> lock(m);
                pass1 = !pass1;
                //std::cout << "Switching light on road. Light is on for direction " << (pass1 ? "1-3" : "2-4") << "\n";
                cv.notify_all();
            }
        }
    }

    void TrafficLight :: requestStop() {
      lightThread.request_stop() ;
    }

    bool TrafficLight :: canPass(int direction) {
        std::lock_guard<std::mutex> lock(m);
        return (pass1 && (direction == 1 || direction == 3)) || (!pass1 && (direction == 2 || direction == 4));
    }

    bool TrafficLight :: waitToPass(int direction) {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this, direction] { return canPass(direction); });
        return true;
    }




    Road :: Road(size_t timeToSwicthLight) : m_light(timeToSwicthLight) {}

    Road :: ~Road() {
       while(!notPassedCars.empty() ) {
        auto car=notPassedCars.front() ;
        std::cout << "Car " << car.carID <<" could not pass\n" ;
        notPassedCars.pop() ;
       }
    }


    void Road :: carPassing(const Car& obj) {
        auto dir = obj.direction;
        {
            std::lock_guard<std::mutex> lock(cout_mutex);

            if (totalCarChecked == totalCars) {
                m_light.requestStop() ;
            }

            if ( !m_light.waitToPass(dir) || (obj.timeToPass)>(m_light.getTimeToSwitch() ) ) {
                std::cerr << "Error: Car " << obj.carID << " could not pass.\n" ;
                notPassedCars.push(obj) ;
                ++totalCarChecked;
                return;
            }

            if (dir == 1) {
                direction1.acquire();
            } else if (dir == 2) {
                direction2.acquire();
            } else if (dir == 3) {
                direction3.acquire();
            } else if (dir == 4) {
                direction4.acquire();
            }

            std::cout << "Car " << obj.carID << " is passing to " << obj.direction << "...\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(obj.timeToPass));

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Car " << obj.carID << " passed." << std::endl;

            if (dir == 1) {
                direction1.release();
            } else if (dir == 2) {
                direction2.release();
            } else if (dir == 3) {
                direction3.release();
            } else if (dir == 4) {
                direction4.release();
            }

            ++totalCarChecked;
        }
    }




    RoadSimulation::RoadSimulation(std::vector<size_t>& timeArrival, std::vector<size_t>& cars, std::vector<size_t>& carsDirection) : m_road1(10) , m_road2(10) {
        if (timeArrival.size() != cars.size() || timeArrival.size() != carsDirection.size() || cars.size() != carsDirection.size()) {
            throw std::invalid_argument("Given vectors are invalid\n");
        }

        for (size_t i{}; i != cars.size(); ++i) {
            m_cars.emplace_back(Car{ cars[i], timeArrival[i], carsDirection[i] });
        }

        Road::totalCars = m_cars.size();
    }

    void RoadSimulation :: operator()() {
        for (auto& car : m_cars) {
            threads.emplace_back(std::jthread([this, &car](std::stop_token st) {
                if (car.direction == 1 || car.direction == 3)
                    m_road1.carPassing(car);
                else if (car.direction == 2 || car.direction == 4)
                    m_road2.carPassing(car);
            }));
        }
    }

    size_t Road::totalCarChecked{};
    size_t Road::totalCars{};

int main() {
    std::vector<size_t> timeArrival = { 1, 2, 3 , 11};
    std::vector<size_t> cars = { 1, 2, 3 , 4  };
    std::vector<size_t> carsDirection = { 1, 2, 3 , 1 };

    RoadSimulation simulation(timeArrival, cars, carsDirection);
    simulation();

    return 0;
}
