#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>
#include <sstream>
#include <iostream>

//this class will act as the translator that transform the data(HTTP request) reveived form client(browser)
//into clean and structure c++ object
//HTTP request look like this
/*GET / HTTP/1.1 
\r\n
Host: www.example.com
\r\n
User-Agent: Mozilla/5.0
\r\n
Accept: text/html
\r\n
\r\n
*/


class HTTPRequest
{
    private:
        std::string method;
        std::string uri;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        bool is_valid_request;
    
        public:
            HTTPRequest();
            bool parse(const std::string& raw_request);
            bool isValid() const;
            std::string getMethod() const;
            std::string getUri() const;
            std::string getVersion() const;
            std::string getHeader(const std::string& key) const;
            std::string getBody() const;
};

#endif