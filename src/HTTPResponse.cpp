#include "HTTPResponse.hpp"
#include <sstream>

HTTPResponse::HTTPResponse()
{

}

void HTTPResponse::setStatusCode(int code,const std::string& message)
{
    status_code = code;
    status_message = message;
}

void HTTPResponse::setHeader(const std::string& key,const std::string& value)
{
    headers[key] = value;
}

void HTTPResponse::setBody(const std::string& content)
{
    body = content;
}

std::string HTTPResponse::toString(bool includeBody) const
{
    std::ostringstream response;

    response << "HTTP/1.1 "
             << status_code << " "
             << status_message <<  "\r\n";

    for(const auto& header: headers)
    {
        response << header.first << ": "
                 << header.second << "\r\n";
    }

    response << "Content-Length: " << body.size() << "\r\n";
    response << "\r\n";

    if(includeBody)
        response.write(body.data(),body.size());

    return response.str();

}