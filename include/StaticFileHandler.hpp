#pragma once

#include <string>

class StaticFileHandler
{
    public:
        explicit StaticFileHandler(const std::string& root);

        bool serve(const std::string& uri, std::string& body, std::string& contentType) const;

    private:
        std::string publicRoot;

        std::string getMimeType(const std::string& path) const;
};