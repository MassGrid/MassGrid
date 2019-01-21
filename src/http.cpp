#include "http.h"
#include <cstdio>
#include "util.h"
int HttpRequest::HttpGet(const string &ip,const int &port,const string &page,const string &strData,const string strSocket)  
{  
    return HttpRequestExec("GET", ip, port, page, strData,strSocket);  
}  

int HttpRequest::HttpDelete(const string &ip,const int &port,const string &page,const string &strData,const string strSocket)  
{  
    return HttpRequestExec("DELETE", ip, port, page, strData,strSocket);  
}  
  
int HttpRequest::HttpPost(const string &ip,const int &port,const string &page,const string &strData,const string strSocket)  
{  
    return HttpRequestExec("POST", ip, port, page, strData,strSocket);  
}  
int HttpRequest::HttpGet()  
{  
    return HttpRequestExec("GET", ip, port, url, strData,strSocket);  
}  
int HttpRequest::HttpDelete()  
{  
    return HttpRequestExec("DELETE", ip, port, url, strData,strSocket);  
}
int HttpRequest::HttpPost()  
{ 
    return HttpRequestExec("POST", ip, port, url, strData,strSocket);
} 
int HttpRequest::GetContentSize(boost::asio::streambuf &response)
{
    std::string head;
    std::istream response_data(&response);
    std::getline(response_data, head);
    int contLength=std::stol(head.c_str(),nullptr,16);
    return contLength;
}
int HttpRequest::UnixSocket(const string &strMethod,const string &page, const string &strData,boost::asio::streambuf &strUnixHead,const string strSocket)
{
    LogPrint("dockerapi","HttpRequest::HttpRequestExec using Unix socket\n");
    unsigned int status_code;
    try
    {
        boost::asio::io_service io_service;
        // Multiplex io
        if(io_service.stopped())
            io_service.reset();
        boost::asio::local::stream_protocol::socket socket(io_service);
        boost::asio::local::stream_protocol::endpoint ep(strSocket);
        socket.connect(ep);

        // Send the request.
        boost::asio::write(socket, strUnixHead);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
            strResponse = "Invalid response";
            return -2;
        }
        // If the server returns non-200, thinks it is wrong, it does not support jumps such as 301/302.
        if (status_code < 0){
            strResponse = "Response returned with status code != 200 ";
            return status_code;
        }

        // header
        std::string header;
        std::vector<string> headers;        
        int contLength=0;
        while (std::getline(response_stream, header) && header != "\r"){
            headers.push_back(header);
            if(header.find("Content-Length")!=string::npos){
                sscanf(header.c_str(),"%*s %d",&contLength);
            }
        }
        // Read all remaining data
        boost::system::error_code error;
        while (boost::asio::read(socket, response,boost::asio::transfer_at_least(1), error))
        {           
        }

        //Responsive data
        if (response.size()>0){
            std::istream response_stream(&response);
            if(contLength==0){
                contLength=GetContentSize(response);
            }
            std::istreambuf_iterator<char> eos;
            std::string data = string(std::istreambuf_iterator<char>(response_stream), eos);
            if(contLength==0 || contLength>data.size()) contLength=data.size();
            strResponse=data.substr(0,contLength);//getJson(data);
            return status_code;           
        }

        if (error != boost::asio::error::eof){
            strResponse = error.message();
            return -3;
        }
    }catch(std::exception& e){
        strResponse = e.what();
        return -4;  
    }      
    return status_code;  
}
//Execute http request  
int HttpRequest::HttpRequestExec(const string &strMethod, const string &ip, const int &port,const string &page, const string &strData,const string strSocket)
{
    //Determine whether the URL is valid 
    if(ip.empty()||ip.size()==0){  
        LogPrint("dockerapi","HttpRequest::HttpRequestExec URL is NULL\n");   
        return -5;  
    }
      
    //Limit URL length  
    if(URLSIZE < page.size()) {  
         LogPrint("dockerapi","HttpRequest::HttpRequestExec Url cannot exceed %d\n",URLSIZE);   
        return -6;  
    }
    string host=ip,ports=std::to_string(port);
    string hostUrl=host+":"+ports;
    boost::asio::streambuf strHttpHead;
    unsigned int status_code;
    //Create an HTTP protocol header  
    if(HttpHeadCreate(strMethod, hostUrl, page, strData,strHttpHead)!=0){
        LogPrint("dockerapi","HttpRequest::HttpRequestExec Header is ULL\n");
        
        return -8;
    }
    if(strSocket.size()>0){
        int ret=UnixSocket(strMethod,page,strData,strHttpHead,strSocket);
        if(ret>=0)
            return ret;
    }
    LogPrint("dockerapi","HttpRequest::HttpRequestExec unix-socket  error, using http\n");
    try
    {
        boost::asio::io_service io_service;
        // Multiplex io
        if(io_service.stopped())
        io_service.reset();

        // Listen to all ip
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, ports);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        
        // Try to connect to one of the ips until it succeeds 
        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        // Send the request.
        boost::asio::write(socket, strHttpHead);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
            strResponse = "Invalid response";
            return -2;
        }
        // If the server returns non-200, thinks it is wrong, it does not support jumps such as 301/302.
        if (status_code < 0){
            strResponse = "Response returned with status code != 200 ";
            return status_code;
        }

        // header
        std::string header;
        std::vector<string> headers;        
        int contLength=0;
        while (std::getline(response_stream, header) && header != "\r"){
            headers.push_back(header);
            if(header.find("Content-Length")!=string::npos){
                sscanf(header.c_str(),"%*s %d",&contLength);
            }
        }
        // Read all remaining data
        boost::system::error_code error;
        while (boost::asio::read(socket, response,boost::asio::transfer_at_least(1), error))
        {           
        }

        //Responsive data
        if (response.size()>0){
            std::istream response_stream(&response);
            if(contLength==0){
                contLength=GetContentSize(response);
            }
            std::istreambuf_iterator<char> eos;
            std::string data = string(std::istreambuf_iterator<char>(response_stream), eos);
            if(contLength==0 || contLength>data.size()) contLength=data.size();
            strResponse=data.substr(0,contLength);//getJson(data);
            return status_code;           
        }

        if (error != boost::asio::error::eof){
            strResponse = error.message();
            return -3;
        }
    }catch(std::exception& e){
        strResponse = e.what();
        return -4;  
    }      
    return status_code;  
}
  
  
//Build HTTP headers  
int HttpRequest::HttpHeadCreate(const string &strMethod, const string &strHostPort, const string &page, const string &strData, boost::asio::streambuf &request)  
{  
    //     // Form the request. We specify the "Connection: close" header so that the
//     // server will close the socket after transmitting the response. This will
//     // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request);
    request_stream << strMethod<<" "<< page;
    if(strMethod=="GET" && !strData.empty()){
        request_stream << "?" << strData;
    }
    request_stream << " HTTP/1.1\r\n";
    request_stream << "Host: " << strHostPort<< "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Content-Type: application/x-www-form-urlencoded\r\n";
    request_stream << "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n";
    request_stream << "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:62.0) Gecko/20100101 Firefox/62.0\r\n";
    request_stream << "Cache-Control: no-cache\r\n";
    request_stream << "Connection: close\r\n";
    if(strMethod == "POST"){
        request_stream << "Content-Length: " << strData.length() << "\r\n";
        request_stream << "\r\n";
        request_stream << strData;
    }
    request_stream << "\r\n";  
      
    return 0;   
}
std::string getJson(std::string str)
{
    int64_t size=str.size();
    int i,j;
    for(i=0;i<size && str[i]!='{' && str[i]!='[' ;i++);
    for(j=size-1;j>=0 && str[j]!='}' && str[j]!=']';j--);
    return str.substr(i,j-i+1);
}
