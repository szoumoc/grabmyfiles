#include "server/service/fileSharer.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace server::service
{
    void runfileSender(int client_fd, const std::string &filePath){
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            ::close(client_fd);
            return;
        }
        const size_t buffer_size = 4096; //size_t is an unsigned integer type that is used to represent the size of objects in bytes. It is typically defined in the <cstddef> header and is used for memory management and array indexing. In this code, we use size_t to define the buffer_size variable, which specifies the size of the buffer we will use to read data from the file and send it to the client. Using size_t ensures that we can handle large files and buffers without running into issues with signed integer overflow or negative values.
        // why 4096? The buffer size of 4096 bytes (4 KB) is a common choice for file transfer applications because it strikes a good balance between performance and memory usage. A buffer of this size allows for efficient reading and writing of data without consuming too much memory, while also providing enough capacity to reduce the number of read/write operations needed to transfer large files. Additionally, many file systems and network protocols are optimized for block sizes around this size, which can further improve performance during file transfers.
        char buffer[buffer_size];
        while (file.read(buffer, buffer_size) || file.gcount() > 0)
        // The loop condition file.read(buffer, buffer_size) || file.gcount() > 0 is used to read data from the file in chunks of buffer_size bytes and send it to the client. The file.read function attempts to read buffer_size bytes from the file into the buffer. If it successfully reads the full buffer size, it returns a reference to the stream (which evaluates to true), and we proceed to send the data. However, if we reach the end of the file and there are fewer than buffer_size bytes left, file.read will return false, but we can still check how many bytes were actually read using file.gcount(). If gcount() returns a value greater than 0, it means we have some remaining data that needs to be sent to the client, even though we didn't fill the entire buffer. This allows us to ensure that all data from the file is sent, including any final partial chunk at the end of the file.
        {
            ssize_t bytes_sent = ::send(client_fd, buffer, file.gcount(), 0);
            // file.gcount() returns the number of bytes that were actually read into the buffer by the last call to file.read. This is important because if we reach the end of the file and there are fewer than buffer_size bytes left, we need to know exactly how many bytes were read so that we can send only that amount to the client. By using file.gcount() as the length parameter in the send function, we ensure that we are sending the correct number of bytes from the buffer, even if it's less than the full buffer size.
            if (bytes_sent < 0)
            {
                std::cerr << "Failed to send data: " << strerror(errno) << std::endl;
                break;
            }
        }
        std::cout << "File transfer completed for: " << filePath << std::endl;
        ::close(client_fd);
    }
    // The offerFile function is responsible for generating a unique port number and associating it with the provided file path. It uses a loop to continuously generate random port numbers until it finds one that is not already in use (i.e., not present in the available_files_ map). Once a unique port number is found, it adds an entry to the available_files_ map with the port as the key and the file path as the value, and then returns the port number.
    int FileSharer::offerFile(const std::string &filePath)
    {
        while (true)
        {
            const int port = server::utils::generateCode();
            if (available_files_.find(port) == available_files_.end())
            {
                available_files_.emplace(port, filePath);
                return port;
            }
        }
    }

    void FileSharer::startFileServer(int port){
        const auto it = available_files_.find(port);
        if (it == available_files_.end())
        {
            std::cerr<<"Port not found: " << port << std::endl;
            return;

        }
        const std::string filePath = it->second;
        // Create a socket
        int server_fd = ::socket(AF_INET, SOCK_STREAM, 0); //what is this? AF_INET is the address family for IPv4, SOCK_STREAM indicates that we want a TCP socket, and 0 means we are using the default protocol for this type of socket (which is TCP).
        if (server_fd < 0 ){
            std::cerr<<"Failed to create socket: " << strerror(errno) << std::endl;
            return;
        }
        int options = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(options)) < 0) {
            // setsockopt is a system call used to set options on a socket. In this case, we are setting the SO_REUSEADDR option, which allows the socket to be bound to an address that is already in use. This is useful for allowing the server to restart and bind to the same port without waiting for the previous socket to be fully closed.
            // what is the use of options variable? The options variable is an integer that we set to 1 to enable the SO_REUSEADDR option. When we call setsockopt, we pass a pointer to this variable, and the function will read its value to determine whether to enable or disable the option. In this case, since options is set to 1, it indicates that we want to enable the SO_REUSEADDR option for the socket.
            // SO_REUSEADDR allows a socket to bind to an address that is already in use, which can be useful for quickly restarting a server without waiting for the previous socket to be fully closed. If setsockopt fails, it returns -1 and sets errno to indicate the error. In this case, we print an error message that includes the reason for the failure (using strerror(errno) to get a human-readable string for the error code), close the socket to free up resources, and return from the function to prevent further execution.
            // SOL_SOCKET is the level at which the option is defined (in this case, it indicates that the option is a socket-level option), SO_REUSEADDR is the name of the option we want to set, &options is a pointer to the value we want to set for the option (in this case, we want to enable it by setting it to 1), and sizeof(options) specifies the size of the options variable.
            // If setsockopt fails, we print an error message and close the socket to free up resources before returning from the function. This ensures that we don't leave open sockets that could lead to resource leaks.
            std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
            ::close(server_fd);
            return;
        }
        sockaddr_in address{};
        // sockaddr_in is a structure used to specify an endpoint address for IPv4 communication. It contains fields for the address family (sin_family), the IP address (sin_addr), and the port number (sin_port). In this code, we initialize the sockaddr_in structure to set up the server's listening address and port.
        address.sin_family = AF_INET; // AF_INET is a constant that represents the address family for IPv4. By setting sin_family to AF_INET, we indicate that we want to use IPv4 addresses for our socket communication.
        address.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY is a constant that represents any IP address on the local machine. When we set the sin_addr.s_addr field to INADDR_ANY, we are indicating that the socket should listen on all available network interfaces.
        //htonl is a function that converts a 32-bit integer from host byte order to network byte order. In this case, we use htonl to convert the INADDR_ANY constant to the correct byte order for network communication. This ensures that the IP address is correctly formatted when the socket is used for communication over the network.
        address.sin_port = htons(static_cast<uint16_t>(port)); // htons is a function that converts a port number from host byte order to network byte order. This is necessary because different machines may use different byte orders, and network protocols typically use a standard byte order (big-endian) for communication. By using htons, we ensure that the port number is correctly formatted for network communication regardless of the underlying architecture of the machine.
        // what this section basically do is to set up the server's listening address and port. We specify that we want to use IPv4 addresses (AF_INET), listen on all available network interfaces (INADDR_ANY), and bind to the specified port number (converted to network byte order using htons). This allows the server to accept incoming connections on the specified port from any network interface on the machine.
        if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            // bind is a system call used to associate a socket with a specific local address and port. In this case, we are binding the server socket (server_fd) to the address and port specified in the sockaddr_in structure (address). The bind function takes three parameters: the socket file descriptor (server_fd), a pointer to the sockaddr structure that contains the address information (reinterpret_cast<sockaddr*>(&address)), and the size of the address structure (sizeof(address)).
            // reinterpret_cast is a C++ operator that allows us to convert one pointer type to another. In this case, we are converting the pointer to sockaddr_in (which is the type of address) to a pointer to sockaddr (which is the type expected by the bind function). This is necessary because the bind function expects a pointer to a generic sockaddr structure, but we have a more specific sockaddr_in structure that contains the necessary information for our IPv4 address.
            std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
            ::close(server_fd);
            return;
        }
        if (::listen(server_fd, SOMAXCONN) < 0) {
            // listen is a system call used to mark a socket as a passive socket that will be used to accept incoming connection requests. In this case, we are marking the server socket (server_fd) as a listening socket with a backlog of SOMAXCONN, which is a constant that specifies the maximum number of pending connections that can be queued up before the server starts rejecting new connection attempts.
            std::cerr << "Failed to listen on socket: " << strerror(errno) << std::endl;
            ::close(server_fd);
            return;
        }
        std::cout << "Ready to serve on port " << port << " for file: " << filePath << std::endl;
        sockaddr_in client_address{};
        socklen_t client_address_len = sizeof(client_address);
        int client_fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_address), &client_address_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
            ::close(server_fd);
            return;
        }
        std::cout << "Client connected: " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << std::endl;
        char client_ip[INET_ADDRSTRLEN]{};
        const char *ip = ::inet_ntop(AF_INET, &client_address.sin_addr, client_ip,
                                     sizeof(client_ip));
                                     // inet_ntop is a function that converts an IP address from binary form (in this case, the sin_addr field of the client_address structure) to a human-readable string format. The parameters are as follows: AF_INET specifies that we are working with IPv4 addresses, &client_address.sin_addr is a pointer to the binary IP address that we want to convert, client_ip is a buffer where the resulting string will be stored, and sizeof(client_ip) specifies the size of the buffer to ensure that we don't overflow it. The function returns a pointer to the resulting string (client_ip) on success, or nullptr on failure. In this code, we use inet_ntop to convert the client's IP address to a readable format for logging purposes when a client connects to the server.
        std::cout << "Client connected: " << (ip ? ip : "?") << '\n';
        std::thread(runfileSender, client_fd, filePath).detach(); 
        // detach is a member function of the std::thread class that allows a thread to run independently from the thread object that created it. When you call detach on a thread, it means that the thread will continue to run in the background even after the thread object goes out of scope. This is useful when you want to start a thread to perform some work and you don't need to wait for it to finish or join it back to the main thread. In this code, we create a new thread to handle sending the file to the client (runfileSender) and then immediately detach it, allowing it to run independently while the main thread can continue accepting new connections or performing other tasks without being blocked by the file sending operation.
    }

} // namespace server::service