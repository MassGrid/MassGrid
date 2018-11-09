#include "http.h"
#include <cstdio>

int HttpRequest::HttpGet(const string& strUrl,const string& page,const string &strData)  
{  
    return HttpRequestExec("GET", strUrl, page, strData);  
}  
  
int HttpRequest::HttpPost(const string &strUrl, const string &page,const string &strData)  
{  
    return HttpRequestExec("POST", strUrl, page, strData);  
}  
int HttpRequest::HttpGet()  
{  
    return HttpRequestExec("GET", host_ip, url, str_data);  
}  
  
int HttpRequest::HttpPost()  
{  
    return HttpRequestExec("POST", host_ip, url, str_data);
} 
  
//Execute http request  
int HttpRequest::HttpRequestExec(const string &strMethod, const string &strUrl, const string &page, const string &strData)
{
    //Determine whether the URL is valid 
    if(strUrl.empty()||strUrl.size()==0){  
        std::cerr <<"URL is NULL"<<std::endl;   
        return -5;  
    }
      
    //Limit URL length  
    if(URLSIZE < strUrl.size()) {  
         std::cerr <<"Url cannot exceed "<<URLSIZE<<std::endl;   
        return -6;  
    }
    string host,port;
    if(chekoutPortFromUrl(strUrl,host,port)!=0){
        return -7;
    }
    string hostUrl=host+":"+port;
    boost::asio::streambuf strHttpHead;
    //Create an HTTP protocol header  
    if(HttpHeadCreate(strMethod, hostUrl, page, strData,strHttpHead)!=0){
        std::cerr<<"Header is ULL"<<std::endl;
        
        return -8;
    }
    try
    {
        boost::asio::io_service io_service;
        // Multiplex io
        if(io_service.stopped())
        io_service.reset();

        // Listen to all ip
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, port);
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
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
            strResponse = "Invalid response";
            return -2;
        }
        // If the server returns non-200, thinks it is wrong, it does not support jumps such as 301/302.
        if (status_code != 200){
            strResponse = "Response returned with status code != 200 ";
            return status_code;
        }

        // header
        std::string header;
        std::vector<string> headers;        
        while (std::getline(response_stream, header) && header != "\r")
            headers.push_back(header);

        // std::cout<<"HttpHeader:\n**************\n";
        // std::cout<<http_version.c_str()<<" "<<status_code<<" "<<status_message.c_str()<<std::endl;
        // for(int i=0;i<headers.size();i++)
        // {
        //     std::cout<<headers[i].c_str()<<std::endl;
        // }
        // std::cout<<"**************\n";

        // Read all remaining data
        boost::system::error_code error;
        while (boost::asio::read(socket, response,boost::asio::transfer_at_least(1), error))
        {           
        }

        //Responsive data
        if (response.size()){
            std::istream response_stream(&response);
            std::istreambuf_iterator<char> eos;
            std::string data = string(std::istreambuf_iterator<char>(response_stream), eos);
            strResponse=getJson(data);
            return 0;           
        }

        if (error != boost::asio::error::eof){
            strResponse = error.message();
            return -3;
        }
    }catch(std::exception& e){
        strResponse = e.what();
        return -4;  
    }      
    return 0;  
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
        request_stream << "\r\n\r\n";
        request_stream << strData;
    }
    request_stream << "\r\n\r\n";  
      
    return 0;   
}  
  
//Get the host address from the HTTP request URL  
string HttpRequest::GetHostAddrFromUrl(const string &strUrl)  
{
    string strHostAddr=strUrl;
    string::size_type index = strHostAddr.find("http://");//Remove http://  
    if(index != string::npos) {  
        strHostAddr = strHostAddr.substr(7);
    } else {
        index = strHostAddr.find("https://");//Remove https:// 
        if(index != string::npos) {  
            strHostAddr = strHostAddr.substr(8);  
        }  
    }
    return strHostAddr;  
}
  
//Get the port number from the HTTP request URL  
int HttpRequest::chekoutPortFromUrl(const string &strUrl,string &host,string &port)
{  
    string strHostPort = GetHostAddrFromUrl(strUrl);
    string::size_type index=strHostPort.rfind(":");
    if(index == string::npos){
        return -1;
    }
    host=strHostPort.substr(0,index);
    port=strUrl.substr(index+1);
    if(host.empty() || port.empty()) {  
        return -1;  
    }
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
