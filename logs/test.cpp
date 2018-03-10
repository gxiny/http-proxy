#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "proxy.h"
#include <thread>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>

//std::mutex mtx;

std::unordered_map<std::string,Http_Response> cache;       //global value cache
void MultiThread(Proxy varg,int ID){
  varg.handle(cache,ID);
}

int main(){

  const char *hostname = NULL;
  const char *port     = "8080";
  int socket_fd;//socket file discriptor
  int status;//store the state of socket func
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    return -1;
  }
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    return -1;
  }
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "Error: cannot bind socket" << std::endl;
    return -1;
  }
  status=listen(socket_fd,1000);
  if (status==-1) {
    std::cerr<<"Error: cannot listen on socket"<<std::endl; 
    return -1;
  }
  int ID = 0;
  while(1){
    ++ID;
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd=accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd==-1) {
      std::cerr<<"Error: cannot accept connection on socket"<<std::endl;
      continue ;
    }
    std::string ip(inet_ntoa(((struct sockaddr_in *)&socket_addr)->sin_addr));
    //std::cout<<"thread"<<std::endl;
    Proxy P(socket_fd,client_connection_fd,ip);
    std::thread(MultiThread,P,ID).detach();
    //std::cout<<ID<<" should print once"<<std::endl;
  }
  close(socket_fd);
  return 0;
}
