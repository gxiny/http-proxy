/*************************************/
/*  get browser request and handle it*/
/*  fix POST issues                  */
/*  last update:3/2/2018             */
/*  version:1.6                      */
/*************************************/

#ifndef __PROXY__H__
#define __PROXY__H__
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <vector>
#include "parse.h"
#include "cache_test.h"
#include "http.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <mutex>
#include <pthread.h>

//pthread_mutex_t lock;
//std::mutex mtx;
int status;
class Proxy{
 private:
  /***main fields***/
  int socket_fd;
  int client_connection_fd;//store the browser file fd
  int BUFF_SIZE=65536;
  std::string req;        //store all the browser's requests
  int port_num;           //store the request's port #
  std::string hostinfo;
  /****************/



  /*********print specific error to the log file**************/
  //status=-100 recv err, -200 send err, -300 select err, -400 connect err
  void print_error(int &ID,int status){
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    if(status==400){
      file<<ID<<" : "<<"ERROR invalid HTTP request\n";
      file<<ID<<" : "<<"Responding HTTP/1.1 400 Bad Request\n";
      file.close();
      return;
    }
    else if(status==502){
      file<<ID<<" : "<<"ERROR invalid Response header\n";
      file<<ID<<" : "<<"Responding HTTP/1.1 502 Bad Gateway\n";
      file.close();
      return;
    }
    else if(status==411){
      file<<ID<<" : "<<"ERROR invalid POST Require header\n";
      file<<ID<<" : "<<"Responding HTTP/1.1 411 Length Required\n";
      file.close();
      return;
    }
    else if(status==-100){
      file<<ID<<" : "<<"ERROR recv() fail\n";
      file.close();
      return;
    }
    else if(status==-200){
      file<<ID<<" : "<<"ERROR send() fail\n";
      file.close();
      return;
    }
    else if(status==-300){
      file<<ID<<" : "<<"ERROR select() fail\n";
      file.close();
      return;
    }
    else if(status==-400){
      file<<ID<<" : "<<"ERROR connect() fail\n";
      file.close();
      return;
    }
    else{
      file.close();
      return;
    }
  }
  /**********************/

  /***************get helper func*******************/
  //get the request from the web browser
  void get_requestHelper(std::unordered_map<std::string,Http_Response> &cache,int &ID){
    std::time_t result = std::time(NULL);  //get current time
    std::string temp;
    int recv_len;
    do{  
      char newbuff[BUFF_SIZE];
      memset(newbuff,0,BUFF_SIZE);
      recv_len=recv(client_connection_fd,newbuff,BUFF_SIZE,0);
      if(recv_len<0){
	print_error(ID,-100);
	return ;
      }
      std::string add(newbuff);
      temp.append(add);
      if(temp.find("\r\n\r\n")!=std::string::npos){
	break;
      }
    }while(recv_len!=0);
    int find_content=-1;
    if((find_content=temp.find("\r\n\r\n"))==std::string::npos||
	temp.size()==0){
	print_error(ID,400);
      return;
      }
    if((find_content=temp.find("POST"))!=std::string::npos){
      if((find_content=temp.find("Content-Length: "))!=std::string::npos){
	std::string before=temp.substr(find_content);
	before=before.substr(16,before.find("\r\n"));
	int post_len=stoi(before);
	temp=temp.substr(0,recv_len);
      }
      else{
	print_error(ID,411);
	return;
      }
    }
    else{
      temp=temp.substr(0,temp.find("\r\n\r\n")+4);
    }
    Parse P(temp);
    port_num=P.port_send();
    std::string origin_host=P.host_send();
    std::string req=temp;
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    std::string logreq=req.substr(0,req.find("\r\n"));
    file<<ID<<" : "<<logreq
        <<" from "<<hostinfo
        <<" @ "<<std::asctime(std::gmtime(&result));
    file.close();
    if(P.method_send()=="CONNECT"){
      handle_connect(origin_host,port_num,ID);
    }
    else if(P.method_send()=="GET"){
      if(cache.find(req)!=cache.end()){
	if((temp.find("no-cache")!=std::string::npos)||(cache[req].revalidation)){
	  std::ofstream file;
	  file.open("/var/log/erss/proxy.log",std::ios::app);
	  file<<ID<<" : "<<"in cache, requires validation\n";
	  file.close();
	  handle_requestHelper(req,origin_host,port_num,cache,ID);
	}
	simple_receive_helper(req,origin_host,port_num,cache,ID);
      }
      else{
	std::ofstream file;      
	file.open("/var/log/erss/proxy.log",std::ios::app);
	file<<ID<<" : "<<"not in cache\n";
	file.close();
	handle_requestHelper(req,origin_host,port_num,cache,ID);
      }
    }
    else{
      handle_requestHelper(req,origin_host,port_num,cache,ID);
    }
    close(client_connection_fd);
  }
  /***********************************************/

  
  /***************handle connect func*************/
  //use to handle to CONNECT request
  void handle_connect(std::string orghost,int P,int &ID){ 
    int origin_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in origin;
    struct hostent *origin_hp;
    origin_hp=gethostbyname(orghost.c_str());
    if(origin_hp==NULL){
      print_error(ID,400);
      close(origin_fd);
      return;
    }
    origin.sin_family=AF_INET;
    origin.sin_port=htons(P);
    memcpy(&origin.sin_addr,origin_hp->h_addr_list[0],origin_hp->h_length);
    int response=connect(origin_fd,(struct sockaddr *)&origin,sizeof(origin));
    if(response<0){
      print_error(ID,-400);
      close(origin_fd);
      return ;
    }
    int check;
    std::string ok("HTTP/1.1 200 Connection Established\r\n\r\n");
    check=send(client_connection_fd,ok.c_str(),strlen(ok.c_str()),0);
    if(check<0){
      print_error(ID,-200);
      close(origin_fd);
      return;
    }
    char buff[BUFF_SIZE];
    memset(buff,0,BUFF_SIZE);
    int highest=(origin_fd>client_connection_fd)?origin_fd:client_connection_fd;
    fd_set readfds;
    while(1){
      FD_ZERO(&readfds);
      FD_SET(origin_fd,&readfds);
      FD_SET(client_connection_fd,&readfds);
      int m=select(highest+1,&readfds,NULL,NULL,NULL);
      if(m<0){
	print_error(ID,-300);
	break ;
      }
      memset(buff,0,BUFF_SIZE); 
      int curr;
      int recv_len;
      if(FD_ISSET(client_connection_fd,&readfds)){
	curr=client_connection_fd;
	recv_len=recv(curr,buff,BUFF_SIZE,0);
	if(recv_len<0){
	  print_error(ID,-100);
	  break;
	}
	if(recv_len==0){
	  close(origin_fd);
	  return;
	}
	int client_send=send(origin_fd,buff,recv_len,0);
	if(client_send<0){
	  print_error(ID,-200);
	  break;
	}
      }
      else if(FD_ISSET(origin_fd,&readfds)){
	curr=origin_fd;
	recv_len=recv(curr,buff,BUFF_SIZE,0);
	if(recv_len<0){
	  print_error(ID,-100);
	  break;
	}
	if(recv_len==0){
	  close(origin_fd);
	  break;
	}
	int original_send=send(client_connection_fd,buff,recv_len,0);
	if(original_send<0){
	  print_error(ID,-200);
	  break;
	}
      }
    }
    close(origin_fd);
    return;
  }
  /***********************************************/

  /***********simple recvive function only to recvive response header*****************/
  // P stands for port number
  // first search cache if in cache no need to connect with server
  // if not in cache call handle_requestHelpe()
  void simple_receive_helper(std::string res,std::string orghost,int P,std::unordered_map<std::string,Http_Response> &cache,int &ID){      
    int origin_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in origin;
    struct hostent *origin_hp;
    origin_hp=gethostbyname(orghost.c_str());
    if(origin_hp==NULL){
      print_error(ID,400);
      close(origin_fd);
      return;
    }
    origin.sin_family=AF_INET;
    origin.sin_port=htons(P);
    memcpy(&origin.sin_addr,origin_hp->h_addr_list[0],origin_hp->h_length);
    int response=connect(origin_fd,(struct sockaddr *)&origin,sizeof(origin));
    if(response<0){
      print_error(ID,-400);
      close(origin_fd);
      return ;
    }
    const char* info=res.c_str();
    send(origin_fd,info,strlen(info),0);
    std::vector<char> newbuff(100);
    int sum=0;
    int target=0;
    int recv_len;
    do{
      char *p = newbuff.data();
      p = p+sum;
      recv_len=recv(origin_fd,p,100,0);
      target += recv_len;
      sum += recv_len;
      std::string ttt(newbuff.begin(),newbuff.end());
      if(ttt.find("\r\n\r\n")!=std::string::npos){
        break;
      }
      newbuff.resize(sum+100);
      if(recv_len<0){
	print_error(ID,-100);
	close(origin_fd);
	return ;   
      }
    }while(recv_len!=0);
    std::string check(newbuff.begin(),newbuff.end());
    if(check.find("\r\n\r\n")==std::string::npos){
      print_error(ID,502);
      close(origin_fd);
      return;
    }
    int a = SearchCache(res,newbuff,cache,client_connection_fd,ID);
    close(origin_fd);
    if(a ==1){
      handle_requestHelper(res,orghost,P,cache,ID);
    }
    else if(a==0){
    }
  }
  /******************************************************************************/



  /****************handler helper func***********/
  //connect with server and get response back to broswer and update cache
  void handle_requestHelper(std::string res,std::string orghost,int P,std::unordered_map<std::string,Http_Response> &cache, int &ID){
    std::string bb = res;
    bb = bb.substr(bb.find("Host:")+6);
    bb = bb.substr(0,bb.find("\r\n"));
    std::string aa = res;
    aa = aa.substr(0,aa.find("\r\n"));
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"Requesting "<<aa<<" from "<<bb<<"\n";
    file.close(); 
    int origin_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in origin;
    struct hostent *origin_hp;
    origin_hp=gethostbyname(orghost.c_str());
    if(origin_hp==NULL){
      print_error(ID,400);
      close(origin_fd);
      return;
    }
    origin.sin_family=AF_INET;
    origin.sin_port=htons(P);
    memcpy(&origin.sin_addr,origin_hp->h_addr_list[0],origin_hp->h_length);
    int response=connect(origin_fd,(struct sockaddr *)&origin,sizeof(origin));
    if(response<0){
      print_error(ID,-400);
      close(origin_fd);
      return ;
    }
    const char* info=res.c_str();
    send(origin_fd,info,strlen(info),0);
    std::vector<char> newbuff(1000);
    int sum=0;
    int target=0;
    int recv_len;
    int size = 1000;
    int end = 0;
    do{
      char *p = newbuff.data();
      p = p+sum;
      recv_len=recv(origin_fd,p,size,0);
      target += recv_len;
      sum += recv_len;
      std::string find_chunk(newbuff.begin(),newbuff.end());
      if(find_chunk.find("chunked")!=std::string::npos){
	handle_chunk(newbuff, origin_fd,recv_len,ID);  
	return;
      }
      if(target == end){
        break;
      }
      Parse_Response P_res(newbuff);
      if((P_res.length_recv())>=0){
	end = P_res.length_recv();
	size = end+P_res.header_length();
	end = size;
	newbuff.resize(size);
        size = size - recv_len;
      }
      else{
        newbuff.resize(sum+1000);
      }
      if(recv_len<0){
	std::cerr<<"Error: recv failed"<<std::endl;
	break ;   
      }
    }while(recv_len!=0);
    std::string check(newbuff.begin(),newbuff.end());
    if(check.find("\r\n\r\n")==std::string::npos){
      print_error(ID,502);
      close(origin_fd);
      return;
    }
    const char *pp = newbuff.data();
    Parse_Response P_res(newbuff);
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"Received "<<P_res.resp_recv()<<" from "<<bb<<"\n";
    file.close();
    if(!((P_res.control_recv()).empty())){
      file.open("/var/log/erss/proxy.log",std::ios::app);
      file<<ID<<" : "<<"NOTE "<<P_res.control_recv()<<"\n";
      file.close();
    }
    if(!((P_res.Etag_recv()).empty())){
      file.open("/var/log/erss/proxy.log",std::ios::app);
      file<<ID<<" : "<<"NOTE "<<P_res.Etag_recv()<<"\n";
      file.close();
    }
    SaveCache(res,target,newbuff,cache,ID);
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"Responding "<< P_res.resp_recv()<<"\n";
    file.close();
    int actual_send;
    actual_send=send(client_connection_fd,pp,target,0);
    if(actual_send<0){
      print_error(ID,-200);
    }
    close(origin_fd);
    //std::cout<<"succeed!"<<std::endl;
  }
  /*********************************************/
 
  /****************handle_chunked_encoding************************/
  //keep receving response and sending them back
  void handle_chunk(std::vector<char> &newbuff, int origin_fd,int former_recv,int ID){ 
    try
     {
       send(client_connection_fd,newbuff.data(),former_recv,0);
       std::string check(newbuff.begin(),newbuff.end());
       while(1){
	 int found;
	 if((found=check.find("0\r\n"))!=std::string::npos){
	   if(check[found-1]=='\n'){
	     //std::cout<<"I need to get rid of chunk\n";
	     break;
	   }
	 }
	 int recv_len=0;
	 std::vector<char> temp(BUFF_SIZE);
	 recv_len=recv(origin_fd,temp.data(),BUFF_SIZE,0);
	 if(recv_len<=0){
	   break;
	 }
	 send(client_connection_fd,temp.data(),recv_len,0);
	 std::string track(temp.begin(),temp.end());
	 check.append(track);
       }
     }
   catch(std::exception &e){
     throw;
     close(origin_fd);
     return;
   }
   close(origin_fd);
   return;
  }
  /*********************************************/
 
 
 
 public:
  /***********constructor**************/
 Proxy(int s,int c,std::string h):socket_fd(s),client_connection_fd(c),hostinfo(h){}
  /************************************/

  /**************two methods***************/
  void handle(std::unordered_map<std::string,Http_Response> &e,int &ID){
    if(e.empty()){
      //std::cout<<"send empty"<<std::endl;
    }
    get_requestHelper(e,ID);
    if(e.empty()){
      //std::cout<<"receive empty"<<std::endl;
    }  
  }
  /***************************************/

  /**************destructor************/
  ~Proxy(){
    //std::cout<<"destruct\n";
  }
  /************************************/
};

#endif
