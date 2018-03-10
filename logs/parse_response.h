#ifndef __PARSE_RESPONSE__H__
#define __PARSE_RESPONSE__H__
#include <iostream>
#include <ostream>
#include <string>
#include <cstdlib>
#include <stdio.h>

/*
  HTTP/1.1 200 OK
  Date: Fri, 23 Feb 2018 15:24:35 GMT
  Server: Apache
  Last-Modified: Fri, 09 Feb 2018 13:02:28 GMT
  ETag: "5a62-119c-564c723f3c100"
  Accept-Ranges: bytes
  Content-Length: 4508
  Connection: close
  Content-Type: text/css

*/


/*
  HTTP/1.1 200 OK
  Date: Fri, 23 Feb 2018 15:26:07 GMT
  Server: Apache
  P3P: CP=" OTI DSP COR IVA OUR IND COM "
  P3P: CP=" OTI DSP COR IVA OUR IND COM "
  Set-Cookie: BAIDUID=3EAD08F913BBAA617E817A69BD437EA3:FG=1; expires=Sat, 23-Feb-19 15:26:07 GMT; max-age=31536000; path=/; domain=.baidu.com; version=1
  Set-Cookie: BAIDUID=3EAD08F913BBAA6127306849C99C1D93:FG=1; expires=Sat, 23-Feb-19 15:26:07 GMT; max-age=31536000; path=/; domain=.baidu.com; version=1
  Last-Modified: Wed, 08 Feb 2017 07:55:35 GMT
  ETag: "1cd6-5480030886bc0"
  Accept-Ranges: bytes
  Content-Length: 7382
  Cache-Control: max-age=1
  Expires: Fri, 23 Feb 2018 15:26:08 GMT
  Vary: Accept-Encoding,User-Agent
  Connection: Keep-Alive
  Content-Type: text/html
*/

class Parse_Response{
 private:
  std::vector<char> response;
  bool Parse_Status(){
    std::string recv_resp(response.begin(),response.end());
    if(recv_resp.find("200 OK")!=std::string::npos){
      return true;
    }
    return false;
  }
  
  
  std::string Parse_Respon(){
    std::string recv_resp(response.begin(),response.end());
    std::string resp = recv_resp.substr(0,recv_resp.find("\r\n"));
    return resp;
  }

  std::string Parse_Date(){
    std::string recv_resp(response.begin(),response.end());
    std::string time;
    if(recv_resp.find("Date:")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("Date:")+6);
      time = recv_resp.substr(0,recv_resp.find("\r\n"));
    }
    return time;
  }

  std::string Parse_Expire(){
    std::string recv_resp(response.begin(),response.end());
    std::string time;
    if(recv_resp.find("Expires:")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("Expires:")+9);
      time = recv_resp.substr(0,recv_resp.find("\r\n"));
    }
    return time;
  }

  std::string Parse_Modified(){
    std::string recv_resp(response.begin(),response.end());
    std::string time;
    if(recv_resp.find("Last-Modified:")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("Last-Modified:")+15);
      time = recv_resp.substr(0,recv_resp.find("\r\n"));
    }
    return time;
  }
  
  std::string Parse_Control(){
    std::string recv_resp(response.begin(),response.end());
    std::string control;
    if(recv_resp.find("Cache-Control: ")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("Cache-Control: "));
      control = recv_resp.substr(0,recv_resp.find("\r\n"));
    }
    return control;
  }
  
  std::string Parse_ETag(){
    std::string recv_resp(response.begin(),response.end());
    std::string Etag;
    if(recv_resp.find("ETag:")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("ETag:"));
      Etag = recv_resp.substr(0,recv_resp.find("\r\n"));
    }
    return Etag;
  }
  
  int Parse_Length(){
    std::string recv_resp(response.begin(),response.end());
    int length = -100;
    if(recv_resp.find("Content-Length:")!=std::string::npos){
      recv_resp = recv_resp.substr(recv_resp.find("Content-Length:")+16);
      recv_resp = recv_resp.substr(0,recv_resp.find("\r\n"));
      length = std::stoi(recv_resp,nullptr,10);
    }
    return length;
  }
  
  /*int Parse_Chunked(){
    std::string recv_resp(response.begin(),response.end());
    int length = -100;
    if(recv_resp.find("chunked")!=std::string::npos){
    recv_resp = recv_resp.substr(recv_resp.find("\r\n\r\n")+4);
    recv_resp = recv_resp.substr(0,recv_resp.find("\r\n"));
    int length = std::stoi(recv_resp,nullptr,10);
    }
    return length;
    }*/
  
  int Header_Length(){
    std::string recv_resp(response.begin(),response.end());
    int length = -100;
    recv_resp = recv_resp.substr(0,recv_resp.find("\r\n\r\n"));
    length = recv_resp.size()+4;
    return length;
  } 
  
 public:
 Parse_Response(std::vector<char> s):response(s){}
  bool status_recv(){
    return Parse_Status();
  }
  std::string expire_recv(){
    return  Parse_Expire();
  }
  std::string modified_recv(){
    return  Parse_Modified();
  }
  std::string date_recv(){
    return  Parse_Date();
  }
  std::string control_recv(){
    return Parse_Control();
  }
  std::string Etag_recv(){
    return Parse_ETag();
  }
  std::string resp_recv(){
    return Parse_Respon();
  }
  int length_recv(){
    return Parse_Length();
  }
  int header_length(){
    return Header_Length();
  }
   
};
#endif












