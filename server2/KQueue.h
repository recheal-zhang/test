#ifndef _KQUEUE_H_
#define _KQUEUE_H_

//#include "NonCopyable.h"

//const int kReadEvent = 1;
//const int kWriteEvent = 2;


#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include <iostream>
#include <string.h>

#include "Tool.h"

#define kReadEvent 1
#define kWriteEvent 2
#define THREAD_NUM 10

//线程参数
static unsigned int threadPara[THREAD_NUM][8];
pthread_t threadId[THREAD_NUM];
pthread_mutex_t threadLock[THREAD_NUM];
pthread_cond_t count_nonzero[THREAD_NUM];


//static struct dataPacket{
//    struct kevent ev;
//    struct kevent waitEvent[LINET];
//    int sockNumber[LINET] = {0};
//    int MAX = 0;
//    int epfd = 0;
//}ThreadDataPacket;

void decrementCount(int i){
    pthread_mutex_lock(threadLock+i);
    pthread_cond_wait(count_nonzero + i, threadLock + i);
    pthread_mutex_unlock(threadLock + i);
}

void incrementCount(int i){
    pthread_mutex_lock(threadLock + i);
    pthread_cond_signal(count_nonzero + i);
    pthread_mutex_unlock(threadLock + i);
}

void *serverSocket(unsigned int *param){
    pthread_detach(pthread_self());

    char buf[1024];

    for(;;){
        decrementCount(param[7]);
        int n = 0;
        int fd = param[1];


        while((n = ::read(fd, buf, sizeof(buf))) > 0){
            int rWrite = write(fd, buf, n);
            if(rWrite < 0){
                std::cout << "handleRead write error" << std::endl;
            }
        }
        if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return NULL;

        if(n < 0){
            std::cout << "read error" << std::endl;
        }

        close(fd);

        param[0] = 0;

    }
}

static void initThreadPool(void){
    int tag = 0;
    for(int i = 0; i < THREAD_NUM; i++){
        threadPara[i][0] = 0;   //是否活动
        threadPara[i][7] = i;
        pthread_cond_init(count_nonzero + i, NULL);
        pthread_mutex_init(threadLock + i, NULL);
        tag = pthread_create(threadId + i, NULL, \
                (void *(*)(void *))serverSocket, (void *)(threadPara[i]));
        if(tag != 0){
            std::cout << "pthread create error" << std::endl;
        }
    }
}

class KQueue{
    public:
        KQueue(){}
        KQueue(int port, int kMaxEvent):_port(port), _kMaxEvent(kMaxEvent){
        }
        ~KQueue(){}

        void print(){
            std::cout << "hello" << std::endl;
        }
        void print2();
        void updateEvents(int efd, int fd, int events, bool modify);
        void kqueueMain();
        void handleAccept(int efd, int fd);
        void handleRead( int fd);
        void handleWrite(int efd, int fd);
        void loop(int efd, int listenFd, int waitms);

    private:
        short _port;
        int _kMaxEvent;
};


void KQueue::print2(){
    std::cout << "hell2" << std::endl;
}

void KQueue::updateEvents(int efd, int fd, int events, bool modify){
    struct kevent ev[2];
    int n = 0;
    if(events & kReadEvent){
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, \
                (void *)(intptr_t)fd);
    }
    else if(modify){
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_DELETE, 0, 0,\
                (void *)(intptr_t)fd);
    }

    if(events & kWriteEvent){
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, \
                (void *)(intptr_t)fd);
    }
    else if(modify){
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, \
                (void *)(intptr_t)fd);
    }

    int re = kevent(efd, ev, n, NULL, 0, NULL);
    if(re < 0){
        std::cout << "kevent failed" << std::endl;
    }
}

void KQueue::kqueueMain(){

    initThreadPool();
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    std::cout << listenfd << std::endl;
    if(listenfd < 0){
        std::cout << "socket error" << std::endl;
    }



    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(124);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    const char *IP = "127.0.0.1";
//    inet_pton(AF_INET, IP, &(addr.sin_addr));

   //TODO:总是出现bind错误，把地址设为可重用
    int on = 1;
    int se = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&on, sizeof(on));
    if(se < 0){
        std::cout << "set sock error" << std::endl;
    }

    int re = bind(listenfd, (struct sockaddr *)&addr,  sizeof(struct sockaddr));
    std::cout << re << std::endl;
    if(re < 0){
        std::cout << "bind error" << std::endl;
    }

    int lis = listen(listenfd, 10);
    if(lis < 0){
        std::cout << "listen error" << std::endl;
    }

    std::cout << "listen done" << std::endl;

     int epollfd = kqueue();
    if(epollfd < 0){
        std::cout << "kqueue error" << std::endl;
        exit(-1);
    }


    setNonBlock(listenfd);
    updateEvents(epollfd, listenfd, kReadEvent, false);




    //updateEvents(epollfd, listenfd, kReadEvent, false);


    //实际应用应当注册信号处理函数，退出时清理资源
    for(;;){
        loop(epollfd, listenfd, 1000);
    }
}

void KQueue::handleAccept(int efd, int fd){
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int acfd = accept(fd, (struct sockaddr *)&raddr, &rsz);
    if(acfd < 0){
        std::cout << "handle accept error" << std::endl;
    }
    std::cout << "accept done" << std::endl;

    sockaddr_in peer;
    socklen_t peerLen = sizeof(peer);
    int r = getpeername(acfd, (sockaddr *)&peer, &peerLen);
    if(r < 0){
        std::cout << "getpeername error" << std::endl;
    }

    setNonBlock(acfd);
    updateEvents(efd, acfd, kReadEvent | kWriteEvent, false);
    std::cout << "accpet & updateEvents done" << std::endl;
}

void KQueue::handleRead(int fd){
    char buf[4096];
    int n = 0;

    while((n = ::read(fd, buf, sizeof(buf))) > 0){
        int rWrite = write(fd, buf, n);
        if(rWrite < 0){
            std::cout << "handleRead write error" << std::endl;
        }
    }
    if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;

    if(n < 0){
        std::cout << "read error" << std::endl;
    }

    close(fd);
}


void KQueue::handleWrite(int efd, int fd){
    //实际应用中当实现数据可写时写出数据，无数据可写时关闭可写时间
    updateEvents(efd, fd, kReadEvent, true);
}

void KQueue::loop(int efd, int listenFd, int waitms){
    struct timespec timeout;
    timeout.tv_sec = waitms / 1000;
    timeout.tv_nsec = (waitms % 1000) * 1000 * 1000;

    struct kevent activeEvs[_kMaxEvent];
    int n = kevent(efd, NULL, 0, activeEvs, _kMaxEvent, &timeout);

    for(int i = 0; i < n; i++){
        int fd = (int)(intptr_t)activeEvs[i].udata;
        int events = activeEvs[i].filter;

        if(events == EVFILT_READ){
            if(fd == listenFd){
                handleAccept(efd, fd);
            }
            else{
                //handleRead(fd);
                for(int j = 0; j < THREAD_NUM; j++){
                    if(0 == threadPara[j][0]){
                        threadPara[j][0] = 1;
                        threadPara[j][1] = fd;
                        incrementCount(j);
                        break;
                    }
                }
            }
        }

        else if(events == EVFILT_WRITE){
            handleWrite(efd, fd);
        }

        else{
            std::cout << "known events" << std::endl;
        }
    }
}
#endif
