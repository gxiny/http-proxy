#ifndef __HTTP__H__
#define __HTTP__H__
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>

class Http_Response{
 public:
  //std::string response;
  std::vector<char> data;
  std::string expire_time;
  std::string create_time;
  std::string Etag;
  bool revalidation;
  int size;
};
#endif