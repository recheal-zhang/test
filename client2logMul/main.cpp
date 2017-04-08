/*
 *简单TCP通信
 获取时延
 map显示时延
 * */
#include <iostream>
#include <map>
#include <sstream>
#include <fstream>
#include <limits>
#include <string>
#include <queue>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#define eps 1e-8

using namespace std;

void fileWrite( const string &current, const string &min, \
        const string &max, const string &ms100, const string &ms100plus);


//生产者消费者模式
pthread_mutex_t threadLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threadCond = PTHREAD_COND_INITIALIZER;

struct FileWriteMsg{
    string current;
    string min;
    string max;
    string ms100;
    string ms100plus;
};

std::queue<FileWriteMsg> msgQue;

void *showMsgThread(void *){
    while(true){
        pthread_mutex_lock(&threadLock);
        while(msgQue.empty()){
            pthread_cond_wait(&threadCond, &threadLock);
        }
        pthread_mutex_unlock(&threadLock);

        FileWriteMsg currentMsg = msgQue.front();

       msgQue.pop();

       fileWrite(currentMsg.current, currentMsg.min, currentMsg.max, currentMsg.ms100, currentMsg.ms100plus);

    }
    return NULL;
}


const int PORT = 124;
const char *IP = "127.0.0.1";

std::fstream fs;

void fileWrite( const string &current, const string &min, \
        const string &max, const string &ms100, const string &ms100plus){
    stringstream ss1;
    ss1 << current << "\t" << min << "\t" << max << "\t" << ms100 << "\t" << ms100plus ;
    fs << ss1.str() << endl;
    cout << ss1.str() << endl;

    //fs >> current >> "\t" >> min >> "\t" >> max >> "\t" >> ms100 >> "\t" >> ms100plus >> "\n";

}

string doubleToString(double cost){
    stringstream ss;
    ss << cost;
    return ss.str();
}

int listenInit(){
//TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
#ifdef DEBUG
        std::cout << "socket error" << std::endl;
#endif
        exit(-1);
    }

    int optval;
    setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &optval, sizeof(optval));

    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &serverAddr.sin_addr);

    //TODO:设置为非阻塞
//    int flags = fcntl(sockfd, F_GETFL, 0);
//    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);


    int con = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if(con < 0){
#ifdef DEBUG
        std::cout << "connect error" << std::endl;
#endif
        close(sockfd);
        exit(-1);
    }

    return sockfd;

}
int main(){
    cout << "client start" << endl;

    pthread_t tid;
    pthread_create(&tid, NULL, showMsgThread, NULL);

    int pid = static_cast<int>(getpid());
    stringstream ss;
    ss << "./log/"<< pid << ".log" ;
    string fileName;
    ss >> fileName;
    fs.open(fileName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);

    stringstream sTag;
    sTag << "current\t" << "min\t" << "max\t" << "0~100us\t" << ">100us" << "\n";
    cout << sTag.str() << endl;
    //fs << sTag.str() << endl;

    map<int, int> timeMap;
    timeMap[0] = 0;
    timeMap[1] = 0;


    time_t start, end;
    struct timeval startval;
    struct timeval endval;

    double cost;
    double maxCost = std::numeric_limits<double>::min();
    double minCost = std::numeric_limits<double>::max();
    char buf[20];
    int sockfd = listenInit();
    while(1){
        time(&start);
        gettimeofday(&startval, NULL);
        //TODO:send & recv
        sprintf(buf, "7E45007E");
        int loopNum = 0;
//        for(;loopNum < 10000; loopNum++){
        if(send(sockfd, buf, strlen(buf), 0) < 0){
#ifdef DEBUG
            std::cout << "send error" << std::endl;
#endif
            exit(-1);
        }
        if(recv(sockfd, buf, strlen(buf), 0) < 0){
#ifdef DEBUG
            std::cout << "recv error" << std::endl;
#endif
            exit(-1);
        }
//        }
        time(&end);
        gettimeofday(&endval, NULL);
        cost = (endval.tv_sec - startval.tv_sec) * 1000000 + (endval.tv_usec - startval.tv_usec);
        //cost = difftime(end, start);
        //TODO:get min and max
        if(cost - minCost < eps){
            minCost = cost;
        }
        if(cost - maxCost > eps){
            maxCost = cost;
        }

        int cou = static_cast<int>(cost / 100);
        if(cou == 0){
            timeMap[0] =timeMap[0] + 1;
        }
        else if(cou > 0){
            timeMap[1] = timeMap[1] + 1;
        }

        //TODO: write log
        string currentCostStr = doubleToString(cost);
        string minCostStr = doubleToString(minCost);
        string maxCostStr = doubleToString(maxCost);

        string str0_100 = doubleToString(timeMap[0]);
        string str100_ = doubleToString(timeMap[1]);

        //fileWrite(currentCostStr, minCostStr, maxCostStr, str0_100, str100_);
        //
        FileWriteMsg newMsg;
        newMsg.current = currentCostStr;
        newMsg.min = minCostStr;
        newMsg.max = maxCostStr;
        newMsg.ms100 = str0_100;
        newMsg.ms100plus = str100_;

        pthread_mutex_lock(&threadLock);
        msgQue.push(newMsg);
        pthread_cond_signal(&threadCond);
        pthread_mutex_unlock(&threadLock);




    }

    fs.close();
    return 0;
}
