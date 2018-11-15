#ifndef HTTP_H
#define HTTP_H
  
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
        HttpRequest(std::string _ip,int _port,std::string _url,std::string _strdata):
        ip(_ip),port(_port),url(_url),strData(_strdata){}
        ~HttpRequest(){};
        int HttpGet();          
        int HttpGet(const string &ip,const int &port,const string &page,const string &strData);
        int HttpDelete();          
        int HttpDelete(const string &ip,const int &port,const string &page,const string &strData);
        int HttpPost();
        int HttpPost(const string &ip,const int &port,const string &page,const string &strData);
        std::string getReponseData(){return strResponse;}
    private:  
        int HttpRequestExec(const string &strMethod, const string &ip, const int &port,const string &page, const string &strData);
        int HttpHeadCreate(const string &strMethod, const string &strHostPort, const string &page, const string &strData, boost::asio::streambuf &request); 
    public:
        std::string ip;
        int port;
        std::string url;
        std::string strData;
    private:
        std::string strResponse;


};
std::string getJson(std::string str);
#endif  