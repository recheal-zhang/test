#include <iostream>
#include <string>
#include <queue>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define IPADRESS "127.0.0.1"
#define PORT 124
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 100
#define MAXTHREADNUM 2000

long long queryNum = 0;
pthread_mutex_t queryNumMutexLock = PTHREAD_MUTEX_INITIALIZER;
struct timeval startval;
struct timeval endval;
int cost = 1;

//function declaration
int socket_bind(const char *ip, int port);

void do_epoll(int listenfd);

void sherror(const char *err);

void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf);

void handle_accept(int epollfd, int listenfd);

void do_read(int epollfd, int fd, char *buf);

void do_write(int epollfd, int fd, char *buf);

void add_event(int epollfd, int fd, int state);

void modify_event(int epollfd, int fd, int state);

void delete_event(int epollfd, int fd, int state);


void *show_msg_thread(void *){
    pthread_detach(pthread_self());
    while(true){
//       gettimeofday(&endval, NULL);
//       long long cost = (endval.tv_sec - startval.tv_sec) * 1000000 + (endval.tv_usec - startval.tv_usec);
       pthread_mutex_lock(&queryNumMutexLock);
       int qps = static_cast<int>(queryNum / (2));
       queryNum = 0;
       pthread_mutex_unlock(&queryNumMutexLock);
       std::cout << "qps = " << qps << std::endl;
        sleep(2);
        if(cost == 1){cost = 2;}
        else{cost += 2;}

    }
}


pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threadCond = PTHREAD_COND_INITIALIZER;

struct threadMsg{
    int epollfd;
    int fd;
    char *buf;
    struct epoll_event event;
};

std::queue<threadMsg> msgQueue;


void *recv_send_thread(void *){
    pthread_detach(pthread_self());
    while(true){
        pthread_mutex_lock(&threadLock);
        while(msgQueue.empty()){
            pthread_cond_wait(&threadCond, &threadLock);
        }
        pthread_mutex_unlock(&threadLock);

        threadMsg t = msgQueue.front();
        msgQueue.pop();

        int epollfd = t.epollfd;
        int fd = t.fd;
        char *buf = t.buf;
        struct epoll_event event = t.event;

        //core worker
        int nwrite;
        if((nwrite = write(fd, buf, sizeof(buf))) == -1){
            std::cout << "write error" << std::endl;

            delete_event(epollfd, fd, EPOLLIN);
        }

//        if(event.events & EPOLLIN){
//            do_read(epollfd, fd, buf);
//        }
//        else if(event.events & EPOLLOUT){
//            do_write(epollfd, fd, buf);

    }

    return NULL;
}

void init_threadpool(void){
    int tag = 0;
    pthread_t tid[MAXTHREADNUM];
    for(int i = 0; i < MAXTHREADNUM; i++){
        tag = pthread_create(tid+i, NULL, recv_send_thread, NULL);
        if(tag != 0){
            std::cout << "pthread create error" << std::endl;
        }
    }
}


void add_query_num(){
    pthread_mutex_lock(&queryNumMutexLock);
    queryNum++;
    pthread_mutex_unlock(&queryNumMutexLock);
}


int main(int argc, char **argv){
    int listenfd;
    gettimeofday(&startval, NULL);
    pthread_t showMsgThreadId;
    pthread_create(&showMsgThreadId, NULL, show_msg_thread, NULL);

    init_threadpool();
    listenfd = socket_bind(IPADRESS, PORT);
    listen(listenfd, LISTENQ);
    do_epoll(listenfd);
    return 0;
}

int socket_bind(const char *ip, int port){
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        sherror("socket error");
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        sherror("bind error");
    }
    return listenfd;
}

void do_epoll(int listenfd){
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);

    epollfd = epoll_create(FDSIZE);

    add_event(epollfd, listenfd, EPOLLIN);

    while(true){
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        handle_events(epollfd, events, ret, listenfd, buf);
    }

    close(epollfd);
}


void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf){
    int i;
    int fd;

    for(i = 0; i < num; i++){
        fd = events[i].data.fd;

        if((fd == listenfd) && (events[i].events & EPOLLIN)){
            add_query_num();
            handle_accept(epollfd, listenfd);
        }
        else if((events[i].events & EPOLLIN || events[i].events & EPOLLOUT) && fd > 3){
            //TODO:multithread
            add_query_num();
//            do_read(epollfd, fd, buf);
            if(events[i].events & EPOLLIN){
//                modify_event(epollfd, fd, EPOLLOUT);
//
                int nread;

                if((nread = read(fd, buf, MAXSIZE)) < 0){
                    std::cout << "read error" << std::endl;
                    delete_event(epollfd, fd, EPOLLIN);
                    close(fd);
                }
                else if(nread == 0){
                    delete_event(epollfd, fd, EPOLLIN);
                    close(fd);
                }
                else{
                   pthread_mutex_lock(&threadLock);
                   threadMsg ms;
                   ms.epollfd = epollfd;
                   ms.fd = fd;
                   ms.buf = buf;
                   ms.event = events[i];

                   msgQueue.push(ms);


                    pthread_cond_signal(&threadCond);
                    pthread_mutex_unlock(&threadLock);

                }
            }

            if(events[i].events & EPOLLOUT){
//                modify_event(epollfd, fd, EPOLLIN);
//
                delete_event(epollfd, fd, EPOLLOUT);
            }
//            std::cout << "main thread handle done" << std::endl;
        }

    }
}

void handle_accept(int epollfd, int listenfd){
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    clifd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if(clifd == -1){
        std::cout << "accept error" << std::endl;
    }
    //TODO:get client
    else{
        add_event(epollfd, clifd, EPOLLIN);
    }
}

void do_read(int epollfd, int fd, char *buf){
    int nread;
    nread = read(fd, buf, MAXSIZE);
    if(nread == -1){
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        std::cout << "read error" << std::endl;
    }
    else if(nread == 0){
        std::cout << "client close" << std::endl;
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else{
        //TODO:read msg

        //TODO:modify event
//        modify_event(epollfd, fd, EPOLLOUT);
    }
}

void do_write(int epollfd, int fd, char *buf){
    int nwrite;
    nwrite = write(fd, buf, strlen(buf));
    if(nwrite ==-1){
        std::cout << "write error" << std::endl;
        close(fd);
        delete_event(epollfd, fd, EPOLLOUT);
    }
    else{
//        modify_event(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, MAXSIZE);
}

void add_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

void modify_event(int epollfd, int fd, int state){
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void sherror(const char *err){
#ifdef DEBUG
    std::cout << std::string(err) << std::endl;
#endif
    exit(-1);
}
