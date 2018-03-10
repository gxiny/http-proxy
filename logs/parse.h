#ifndef __PARSE_H__
#define __PARSE_H__
#include <iostream>
#include <ostream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdlib>

class Parse{
 private:
  std::string request;
  std::string Parse_Url(){
    std::string recv_requ = request;
    recv_requ = recv_requ.substr(recv_requ.find(" ")+1);
    std::string url;
    url = recv_requ.substr(0,recv_requ.find(" "));
    return url;
  }
  
  std::string Parse_header(){
    std::string recv_requ = request;
    std::string res = recv_requ.substr(0,recv_requ.find("\r\n\r\n")+4);
    return res;
    }
  
  std::string Parse_Request(){
    std::string recv_requ = request;
    std::string res;
    res = recv_requ.substr(0,recv_requ.find("\r\n")); 
    recv_requ = recv_requ.substr(recv_requ.find("Host: ")+6);
    recv_requ = recv_requ.substr(0,recv_requ.find("\r\n")+2);
    res +="\r\n";
    res += "Host: ";
    res += recv_requ;
    res +="\r\n";
    return res;
  }
    
  std::string Parse_Host(){
    std::string recv_requ = request;
    std::string host;
    recv_requ = recv_requ.substr(recv_requ.find("Host: ")+6);
    host = recv_requ.substr(0,recv_requ.find("\r\n"));
    if(host.find(":")!=std::string::npos){
      host = host.substr(0,host.find(":"));
    }
    return host;
  }
  
  int Parse_Port(){
    std::string recv_requ = request;
    std::string temp;
    int port=80;
    recv_requ = recv_requ.substr(recv_requ.find("Host:")+6);
    recv_requ = recv_requ.substr(0,recv_requ.find("\r\n"));
    if(recv_requ.find(":")!=std::string::npos){
      recv_requ = recv_requ.substr(recv_requ.find(":")+1);
      temp = recv_requ.substr(0,recv_requ.find("\r\n"));
      //std::cout<<"port num "<<temp<<std::endl;
      port = std::stoi(temp);
    }
    return port;
  }
  
  std::string Parse_Method(){
    std::string recv_requ = request;
    std::string method;
    method = recv_requ.substr(0,recv_requ.find(" "));
    return method;
  }
        
 public:
 Parse(std::string s):request(s){
  }
  std::string url_send(){
    return Parse_Url();
  }
  std::string req_send(){
    return Parse_Request();
  }
  std::string host_send(){
    return Parse_Host();
  }
  int port_send(){
    return Parse_Port();
  }
  std::string method_send(){
    return Parse_Method();
  }
  std::string header_send(){
    return Parse_header();
  }
};
#endif
