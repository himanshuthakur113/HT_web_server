#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest()
{
    is_valid_request = false;
}

bool HTTPRequest::parse(const std::string& raw_request)
{
    size_t header_end = raw_request.find("\r\n\r\n");
    if(header_end == std::string::npos)
    {
        return false;
    }

    std::string raw_headers = raw_request.substr(0, header_end + 4);

    if(!parseHeaders(raw_headers))
    {
        is_valid_request = false;
        return false;
    }

    std::string remaining_data = raw_request.substr(header_end + 4);

    auto it = headers.find("content-length");

    if(it != headers.end())
    {
        try
        {
            size_t content_length = std::stoul(it->second);
            
            if(remaining_data.length() < content_length)
            {
                is_valid_request = false;
                return false;
            }

            body = remaining_data.substr(0, content_length);
        }
        catch(...)
        {
            is_valid_request = false;
            return false;
        }
    }
    
    is_valid_request = true;
    return true;
}

bool HTTPRequest::isValid() const 
{ 
    return is_valid_request;
}

std::string HTTPRequest::getMethod() const 
{
    return method; 
}

std::string HTTPRequest::getUri() const
{ 
    return uri; 
}

std::string HTTPRequest::getVersion() const 
{ 
    return version; 
}
    
std::string HTTPRequest::getHeader(const std::string& key) const
{   
    std::string lower_key = key;

    for(char &c: lower_key)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    auto it = headers.find(lower_key);
    if(it != headers.end())
    {
        return it->second;
    }
    return "";
}

std::string HTTPRequest::getBody() const
{
    return body;
}

bool HTTPRequest::parseHeaders(const std::string& raw_headers)
{
    std::istringstream stream(raw_headers);
    std::string line;

    headers.clear();        
    body.clear();

    if(!std::getline(stream, line))  //getline will copy line unitl /n detected
    {
        return false;
    }

    // \r\n is the end of HTTP line
    // \r\n\r\n end of HTTP headers
    // getline will remove the \n but not \r 
    if(!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }

    std::istringstream line_stream(line);

    if(!(line_stream >> method >> uri >> version))  // >> will read until space is detected
    {
        return false;
    }

    if(version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        return false;
    }

    //for headers after the first line
    while(std::getline(stream, line) && line != "\r" && !line.empty())
    {
        if(!line.empty() && line.back() == '\r') //same as before getline remove \n not \r
        {
            line.pop_back();
        }

        size_t colon_pos = line.find(':');
        if(colon_pos == std::string::npos)
        {
            return false;
        }

        std::string key = line.substr(0, colon_pos); //start from zero and extract the char before colon,colon_pass is given as size
        for(char& c:key)    //http header name are case insensitive so  converting  all keys to lowercse
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        std::string value = line.substr(colon_pos + 1); //after colon pos to end

        size_t start = value.find_first_not_of(" \t"); //first char that in not space or tab 
        if(start != std::string::npos)
        {
            value = value.substr(start);
        }
        else
        {
            value.clear();
        }
        headers[key] = value;
    }
    return true;
}