#pragma once
//pragma once is directive to ensure that the header file is included only once in a single compilation, preventing duplicate definitions and potential errors.

#include <string>
#include <unordered_map>

#include "server/utils/uploadUtils.hpp"

namespace server::service
{

    class FileSharer
    {
    public:
        FileSharer() = default; // Default constructor i.e FileSharer() {}. It allows you to create an instance of the FileSharer class without providing any arguments.

        int offerFile(const std::string &filePath);
        void startFileServer(int port);

    private:
        std::unordered_map<int, std::string> available_files_;
    };

} // namespace server::service