#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest()
{
    is_valid_request = false;
}

bool HTTPRequest::parse(const std::string& raw_request)
{
    std::istringstream stream(raw_request); //make the request to a stream like cin
    std::string line;

    if(!std::getline(stream, line))  //getline will copy line unitl /n detected
    {
        is_valid_request = false;
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
        is_valid_request = false;
        return false;
    }

    if(version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        is_valid_request = false;
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
            is_valid_request = false;
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
        
        headers[key] = value;
    }

    if(headers.find("Content-Length") != headers.end())
    {
        try
        {
            size_t content_length = std::stoul(headers["content-length"]);  //stoul convert string into usigned long like "1343" to 1343

            std::string remaining_data;
            std::getline(stream, remaining_data, '\0'); //read until \0 is detected which represent null

            if(remaining_data.length() >= content_length)
            {
                body = remaining_data.substr(0, content_length);
            }   
            else
            {
                is_valid_request = false;
                return false;
            }
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
    auto it = headers.find(key);
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