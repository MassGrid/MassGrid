#ifndef __HTTP__  
#define __HTTP__  
  
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>  
#define __DEBUG__ 
#define BUFSIZE 41000  
#define URLSIZE 1024  
using boost::asio::ip::tcp;
using std::string;
class HttpRequest  
{  
    public:
        HttpRequest(){};
        HttpRequest(std::string _host_ip,std::string _url,std::string _strdata):
        host_ip(_host_ip),url(_url),str_data(_strdata){}
        ~HttpRequest(){};
        int HttpGet();          
        int HttpGet(const string& strUrl,const string& page,const string &strData);
        int HttpPost();
        int HttpPost(const string &strUrl, const string &page,const string &strData);
        std::string getReponseData(){return strResponse;}
    private:  
        int HttpRequestExec(const string &strMethod, const string &strUrl, const string &page, const string &strData);
        int HttpHeadCreate(const string &strMethod, const string &strHostPort, const string &page, const string &strData, boost::asio::streambuf &request); 
        string HttpDataTransmit(char *strHttpHead, const int iSockFd);
        string GetHostAddrFromUrl(const string &strUrl);
        int chekoutPortFromUrl(const string &strUrl,string &host,string &port);
    public:
        std::string host_ip;
        std::string url;
        std::string str_data;
    private:
        std::string strResponse;


};
std::string getJson(std::string str);
#endif  