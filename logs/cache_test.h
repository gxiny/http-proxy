#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <limits.h>
#include <time.h>
#include <unordered_map>
#include "parse.h"
#include "parse_response.h"
#include "http.h"
#include <fstream>
//#include <ctime>


void SaveCache(std::string req, int size, std::vector<char> response,std::unordered_map<std::string,Http_Response> &cache, int ID){
  Parse_Response P_res(response);
  
  if(!P_res.status_recv()){
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"not cacheable because not receive 200 OK\n";
    file.close();
    return;
  }
  if(!(P_res.control_recv()).empty()){
    std::string control = P_res.control_recv();
    if((control.find("no-cache")!=std::string::npos)||(control.find("no-store")!=std::string::npos)){
      std::ofstream file;
      file.open("/var/log/erss/proxy.log",std::ios::app);
      file<<ID<<" : "<<"not cacheable because "<<control<<"\n";
      file.close();
      return;
    }
  }
  if(size>INT_MAX){
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"not cacheable because this web is too large\n";
    file.close();
    return;
  }
  if(cache.size()>100){
    std::string bb = cache.begin()->first;
    bb = bb.substr(bb.find(" ")+1);
    bb = bb.substr(0,bb.find(" "));
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
   
    file<<"(no-id)"<<" : "<<"evicted "<<bb<<"from cache because cache can only store 100 pages\n";
    file.close();
    cache.erase(cache.begin());
  }
  
  Http_Response httpresponse;
  httpresponse.data = response;
  httpresponse.expire_time = P_res.expire_recv();
  httpresponse.create_time = P_res.date_recv();
  httpresponse.size = size;
  httpresponse.revalidation = false;
  if(!(P_res.control_recv()).empty()){
    std::string control = P_res.control_recv();
    if(control.find("must-revalidate")!=std::string::npos){
      std::ofstream file;
      file.open("/var/log/erss/proxy.log",std::ios::app);
      file<<ID<<" : "<<"cached, but requires revalidation\n";
      file.close();
      httpresponse.revalidation = true;
    }
  }
  if(!(P_res.expire_recv()).empty()){
    std::string expiretime = P_res.expire_recv();
    const char* expire = expiretime.c_str();
    struct tm e;
    strptime(expire, "%a, %d %b %Y %H:%M:%S GMT", &e); 
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"cached, but expires at "<<asctime(&e);
    file.close();
   
  }
  else{
    std::ofstream file;
    file.open("/var/log/erss/proxy.log",std::ios::app);
    file<<ID<<" : "<<"cached\n";
    file.close();
  }
  cache[req] = httpresponse;
}

int SearchCache(std::string req,std::vector<char> response,std::unordered_map<std::string,Http_Response> &cache,int fd, int ID){//,int fd
  int get_info = 0;                                //==1 need to connect server and save
  Parse_Response P_res(response);
  std::string current = P_res.date_recv();
  std::string Etag = P_res.Etag_recv();
  if(cache.find(req)!=cache.end()){           //if find
    if(cache[req].Etag != Etag){
      std::ofstream file;
      file.open("/var/log/erss/proxy.log",std::ios::app);
      file<<ID<<" : "<<"in cache, requires validation\n";
      file<<ID<<" : "<<"Responding HTTP/1.1 200 OK\n";
      file.close();                //modify time does not exist, send back
      get_info = 1;
      return get_info;
    }
    std::string create_time = cache[req].create_time;
    const char* create= create_time.c_str();
    struct tm c;
    strptime(create, "%a, %d %b %Y %H:%M:%S GMT", &c);
    time_t create_t = mktime(&c);
    std::string expire_time = cache[req].expire_time;
    if(expire_time.empty()){
      std::string modify_time = P_res.modified_recv();
      if(modify_time.empty()){  
	std::ofstream file;
	const char *p = (cache[req].data).data();
	file.open("/var/log/erss/proxy.log",std::ios::app);
	file<<ID<<" : "<<"in cache, valid\n";
	file<<ID<<" : "<<"Responding HTTP/1.1 200 OK\n";
	file.close();             //modify time does not exist, send back
	send(fd,p,cache[req].size,0);
	return get_info;
      }
      else{
	const char* modify = modify_time.c_str();
	struct tm m;
	strptime(modify, "%a, %d %b %Y %H:%M:%S GMT", &m);
	time_t modify_t = mktime(&m);
	double diff_2 = modify_t - create_t;
	if(diff_2 >0){
	  std::ofstream file;
	  file.open("/var/log/erss/proxy.log",std::ios::app);
	  file<<ID<<" : "<<"in cache, but modified at "<<asctime(&m)<<"\n";
	  file.close();
	  get_info = 1;
	  return get_info;
	}
      }
    }
    else{
      const char* expire = expire_time.c_str();
      struct tm e;
      strptime(expire, "%a, %d %b %Y %H:%M:%S GMT", &e);
      time_t expire_t = mktime(&e);
      double diff_1 = expire_t - create_t;
      if(diff_1 < 0){
	std::ofstream file;
	file.open("/var/log/erss/proxy.log",std::ios::app);
	file<<ID<<" : "<<"in cache, but expires at "<<asctime(&e);
	file.close();
	get_info = 1;
	return get_info;
      }

      if(diff_1 > 0){
	std::string modify_time = P_res.modified_recv();
	if(modify_time.empty()){        //modify time does not exist, send back
	  std::ofstream file;
	  const char* p = (cache[req].data).data();
	  /*char send_back[cache[req].size];
	    memset(send_back,0,cache[req].size);
	    for(int i =0;i<cache[req].size;i++){
	    send_back[i] =  cache[req].data[i];
	    }*/
	  file.open("/var/log/erss/proxy.log",std::ios::app);
	  file<<ID<<" : "<<"in cache, valid\n";
	  file<<ID<<" : "<<"Responding HTTP/1.1 200 OK "<<"\n";
	  file.close();
	  send(fd,p,cache[req].size,0);
	  return get_info;
	}
	else{
	  const char* modify = modify_time.c_str();
	  struct tm m;
	  strptime(modify, "%a, %d %b %Y %H:%M:%S GMT", &m);
	  time_t modify_t = mktime(&m);
	  double diff_2 = modify_t - create_t;
	  if(diff_2 >0){
	    std::ofstream file;
	    file.open("/var/log/erss/proxy.log",std::ios::app);
	    file<<ID<<" : "<<"in cach, but modified at "<<modify_time<<"\n";
	    file.close();
	    get_info = 1;
	    return get_info;
	  }
	  /*char send_back[cache[req].size];
	    memset(send_back,0,cache[req].size);
	    for(int i =0;i<cache[req].size;i++){
	    send_back[i] =  cache[req].data[i];
	    }*/
	  const char *p = (cache[req].data).data();
	  std::ofstream file;
	  file.open("/var/log/erss/proxy.log",std::ios::app);
	  file<<ID<<" : "<<"in cache, valid\n";
	  file<<ID<<" : "<<"Responding HTTP/1.1 200 OK\n";
	  file.close();
	  send(fd,p,cache[req].size,0);
	  return get_info;
	}
      }
    }
  }
}


