#include "server/utils/uploadUtils.hpp"

#include <random>

namespace server::utils
{
    // This function generates a random port number within the dynamic/private port range (49152-65535) using a thread-local random number generator.
    int generateCode()
    {
        static constexpr int kDynamicStartingPort = 49152;
        static constexpr int kDynamicEndingPort = 65535;
        static constexpr int kSpan = kDynamicEndingPort - kDynamicStartingPort;

        thread_local std::mt19937 rng{std::random_device{}()};
        // thread_local is a storage specifier in C++ that indicates that each thread has its own instance of the variable. In this case, rng is a random number generator that is initialized with a random seed from std::random_device. This ensures that each thread generates different random numbers without interference from other threads.
        // here random_device is the seed i.e. it provides a non-deterministic random seed for the random number generator, ensuring that the generated port numbers are different each time the program runs.
        std::uniform_int_distribution<int> dist(0, kSpan - 1);
        // std::uniform_int_distribution is a random number distribution that produces integers uniformly distributed across a specified range. In this case, it generates integers from 0 to kSpan - 1, which corresponds to the range of valid port numbers when added to kDynamicStartingPort.
        return dist(rng) + kDynamicStartingPort;
    }

} // namespace server::utils