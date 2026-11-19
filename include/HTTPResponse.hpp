#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <unordered_map>

class HTTPResponse
{
    private:
        int status_code;
        std::string status_message;
        std::unordered_map<std::string, std::string> headers;
        std::string body;

    public:
        HTTPResponse();

        void setStatusCode(int code,const std::string& message);
        void setHeader(const std::string& key, const std::string& value);
        void setBody(const std::string& content);

        std::string toString(bool includeBody = true) const;
};

#endif