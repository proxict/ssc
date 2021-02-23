#include <ssc/ssc.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <thread>

std::string toStr(const ssc::TimePoint& timePoint) {
    time_t tt = ssc::Clock::to_time_t(timePoint);
    std::stringstream ss;
    ss << ctime(&tt);
    return ss.str();
}

int main() {
    try {
        {
            ssc::Cache<std::string, std::string> cache("myCacheStorage");
            cache.storeValue("key", "value", ssc::Clock::now() + 24h * 365);
            const ssc::TimePoint* expiry = cache.getExpiryTime("key");
            if (expiry) {
                std::cout << "Value stored in key will expire at " << toStr(*expiry) << std::endl;
            } else {
                std::cout << "Value with key key not found." << std::endl;
            }

            cache.curate();
            cache.serialize();
            const std::string* val = cache.getValue("key");
            std::cout << "key: " << *val << std::endl;
        }

        std::this_thread::sleep_for(1s);

        {
            ssc::Cache<std::string, std::string> cache("myCacheStorage");
            cache.deserialize();
            const std::string* val = cache.getValue("key");
            if (val) {
                std::cout << "key: " << *val << std::endl;
            } else {
                std::cout << "No value stored" << std::endl;
            }
        }
    } catch (const ssc::Exception& e) {
        std::clog << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
}
