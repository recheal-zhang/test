#ifndef _TOOL_H_
#define _TOOL_H_
#include <iostream>
#include <fcntl.h>

void setNonBlock(int fd);


void setNonBlock(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0){
        std::cout << "fcntl failed" << std::endl;
    }

    int re = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if(re < 0){
        std::cout << "fcntl failed" << std::endl;
    }
}


#endif
