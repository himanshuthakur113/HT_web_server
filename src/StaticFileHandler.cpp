#include "StaticFileHandler.hpp"

#include <fstream>
#include <sstream>
#include <unordered_map>

StaticFileHandler::StaticFileHandler(const std::string& root) : publicRoot(root)
{

}

std::string StaticFileHandler::getMimeType(const std::string& path) const
{
    static const std::unordered_map<std::string, std::string> mimeTypes = 
    {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".json", "application/json"},
        {".txt", "text/plain"}
    };

    size_t pos = path.find_last_of('.');

    if(pos == std::string::npos)
        return "application/octet-stream";

    std::string extension = path.substr(pos);

    for(char &c:extension)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto it = mimeTypes.find(extension);

    if(it != mimeTypes.end())
        return it->second;

    return "application/octet-stream";
}

bool StaticFileHandler::serve(const std::string& uri, std::string& body, std::string& contentType) const
{
    std::string path = uri.substr(0, uri.find('?'));

    if(path.empty() || path == "/")
        path = "/index.html";

    if(path.find('\\') != std::string::npos)
        return false;
    
    std::stringstream pathStream(path);
    std::string segment;

    while(std::getline(pathStream, segment, '/'))
    {
        if(segment == "." || segment == "..")
            return false;
    }

    std::string filePath = publicRoot + path;

    struct stat fileInfo{};

    if(stat(filePath.c_str(), &fileInfo) != 0)
    {
        return false;
    }

    if(!S_ISREG(fileInfo.st_mode))
    {
        return false;
    }

    std::ifstream file(filePath, std::ios::binary);

    if(!file.is_open())
    {
        perror("[static file handler] File not found");
        return false;
    }

    std::ostringstream filebuffer;
    filebuffer << file.rdbuf();

    if(file.bad())
    {
        return false;
    }

    body = filebuffer.str();
    contentType = getMimeType(filePath);

    return true;
}