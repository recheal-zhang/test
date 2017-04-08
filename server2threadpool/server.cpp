#include <iostream>
#include <unistd.h>
#include <pthread.h>

#include "KQueue.h"

using namespace std;
//KQueue *kq;
/*
void *showQPSThread(void *time_interval){
    pthread_detach(pthread_self());
    time_t end ;
    time(&end);
    double cost = difftime(end, start);
    long long qps = static_cast<long long>(queryNum / cost);
    cout << "qps : " << qps << endl;
    sleep(*(int *)time_interval);
    return NULL;
}


void *epollThread(void *){
    pthread_detach(pthread_self());
    KQueue *kq =  new KQueue(89, 20);

    kq->epoll_main();
    return NULL;
}
*/

int main(void){
  //  time(&start);

  //  int time_interval = 1000;
 //   pthread_t threadShowQPS;
 //   pthread_create(&threadShowQPS, NULL, showQPSThread, &time_interval);

 //   pthread_t threadEpoll;
 //   pthread_create(&threadEpoll, NULL, epollThread, NULL);

    std::cout << "start" << endl;

    KQueue kq(124,24);
    kq.kqueueMain();

    std::cout << "end" << endl;
    return 0;
}

