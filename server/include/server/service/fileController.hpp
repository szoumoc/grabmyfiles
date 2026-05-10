#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include "server/httplib/httplib.hpp"
#include "server/service/fileSharer.hpp"

namespace server::services
{

    class FileController
    {
    public:
        explicit FileController(int port);
        // explicit is a C++ keyword that prevents implicit conversions. It is used in the constructor declaration to indicate that the constructor should not be used for implicit type conversions. In this case, it means that you cannot implicitly convert an int to a FileController object. You must explicitly call the constructor with an int argument to create a FileController instance. This helps to avoid unintended conversions and makes the code clearer and safer.
        // For example, without explicit, you could accidentally write something like FileController controller = 8080; which would compile but might not be what you intended. With explicit, you must write FileController controller(8080); which makes it clear that you are creating a FileController object with the specified port.

        // The destructor is responsible for cleaning up resources when a FileController object is destroyed. In this case, it calls the stop() method to ensure that the server is properly stopped and any associated resources are released. This is important to prevent resource leaks and ensure that the server is gracefully shut down when the FileController object goes out of scope or is explicitly deleted.
        ~FileController();

        void start();
        void stop();

    private:
        // registerRoutes is a private member function that is responsible for setting up the routes or endpoints for the HTTP server. It defines how the server should respond to different HTTP requests (e.g., GET, POST) and which handler functions should be called for each route. This function is typically called during the initialization of the FileController to ensure that the server is ready to handle incoming requests according to the defined routes.
        void registerRoutes();
        // applyCorsHeaders is a private member function that adds Cross-Origin Resource Sharing (CORS) headers to the HTTP response. CORS headers are used to allow or restrict resources on a web server to be accessed by web pages from different domains. This function is typically called before sending a response to ensure that the appropriate CORS headers are included, allowing clients from other origins to access the server's resources if necessary.
        // const after the function declaration indicates that this member function does not modify the state of the FileController object. It can be called on const instances of FileController, and it guarantees that it will not change any member variables or call non-const member functions. This is a common practice to indicate that a function is read-only and does not have side effects on the object's state.
        void applyCorsHeaders(httplib::Response &res) const;
        // handleCorsOrNotFound is a private member function that handles CORS headers and not found responses for HTTP requests. It is typically called when a request does not match any defined routes or when CORS preflight requests are received. The function checks the HTTP method of the request and applies CORS headers to the response. If the request method is OPTIONS, it returns a 204 No Content response, indicating that the preflight request was successful. For other methods, it leaves the response status as 404 Not Found, allowing the client to handle it accordingly. This function helps to ensure that CORS requests are properly handled while also providing a fallback for unmatched routes.
        void handleCorsOrNotFound(const httplib::Request &req, httplib::Response &res) const;

        service::FileSharer fileSharer_;
        httplib::Server server_;
        std::filesystem::path uploadDir_;
        int port_;

        // Rough equivalent to Java executor + server lifecycle control
        std::thread serverThread_;
        std::atomic<bool> running_{false};
    };

} // namespace server::services