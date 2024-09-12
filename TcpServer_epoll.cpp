#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

int main()
{
    // 1.创建套接字
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverFd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 2. 绑定 ip, port
    struct sockaddr_in addr;
    addr.sin_port = htons(8080);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(serverFd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("bind");
        exit(0);
    }
    // 3. 监听
    ret = listen(serverFd, 100);
    if(ret == -1)
    {
        perror("listen");
        exit(0);
    }

    // 创建epoll树
    int epfd = epoll_create(1000);//有关size参数介绍：https://blog.csdn.net/zhoumuyu_yu/article/details/112472419
    if(epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    // 将监听lfd添加到树上
    struct epoll_event serverFdEvt;
    // 检测事件的初始化
    serverFdEvt.events = EPOLLIN ;
    serverFdEvt.data.fd = serverFd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &serverFdEvt);

    struct epoll_event events[1024];

    int s1 = sizeof(events);
    int s2 = sizeof(events[0]);

    char sendBuf[10000];
    int  sendBufLen = 10000;
    memset(sendBuf, 0, sendBufLen);

    // 开始检测
    while(true)
    {
        int nums = epoll_wait(epfd, events, s1/s2, 1);//milliseconds
        printf("serverFd=%d,nums = %d\n", serverFd,nums);

        // 遍历状态变化的文件描述符集合
        for(int i=0; i<nums; ++i)
        {
            int curfd = events[i].data.fd;

            printf("curfd=%d \n",curfd);
            // 有新连接
            if(curfd == serverFd)
            {
                struct sockaddr_in conn_addr;
                socklen_t conn_addr_len = sizeof(addr);
                int connfd = accept(serverFd, (struct sockaddr*)&conn_addr, &conn_addr_len);

                printf("connfd=%d\n",connfd);
                if(connfd == -1)
                {
                    perror("accept\n");
//                    exit(0);
                    break;
                }
                // 将通信的fd挂到树上
                serverFdEvt.events = EPOLLIN | EPOLLOUT;
//                serverFdEvt.events = EPOLLIN;
                serverFdEvt.data.fd  = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &serverFdEvt);
            }
                // 通信
            else
            {
                // 读事件触发, 写事件触发
//                if(events[i].events & EPOLLOUT)
//                {
//                    continue;
//                }

                if(events[i].events & EPOLLIN) {
                    char buf[128];
                    int count = read(curfd, buf, sizeof(buf));
                    if (count <= 0) {
                        printf("client disconnect ...\n");
                        close(curfd);
                        // 从树上删除该节点
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);

                        continue;
                    } else {
                        // 正常情况
                        printf("client say: %s\n", buf);
                    }
                }
                if(curfd > -1)
                {

                    int size = send(curfd, sendBuf, sendBufLen, 0);
                    if (size < 0) {
                        printf("curfd=%d,send error \n",curfd);
                        close(curfd);
                        // 从树上删除该节点
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                        continue;
                    }

                }


            }
        }
    }

    close(serverFd);

    return 0;
}

