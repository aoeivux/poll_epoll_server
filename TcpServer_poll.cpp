#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>

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

    // 4. 等待连接 -> 循环
    // 检测 -> 读缓冲区, 委托内核去处理
    // 数据初始化, 创建自定义的文件描述符集
    struct pollfd fds[1024];
    // 初始化
    for(int i=0; i<1024; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    fds[0].fd = serverFd;

    char sendBuf[10000];
    int  sendBufLen = 10000;
    memset(sendBuf, 0, sendBufLen);

    int maxfd = 0;
    while(true)
    {
        printf("maxfd=%d\n",maxfd);

//        printf("step1\n");
        // 委托内核检测
        ret = poll(fds, maxfd+1, 1);

//        printf("step2\n");
        if(ret == -1)
        {
            perror("select\n");
            break;
        }

        // 检测的度缓冲区有变化
        // 有新连接
        if(fds[0].revents & POLLIN)
        {

            struct sockaddr_in conn_addr;
            socklen_t conn_addr_len = sizeof(addr);
            // 这个accept是不会阻塞的
            int connfd = accept(serverFd, (struct sockaddr*)&conn_addr, &conn_addr_len);
            // 委托内核检测connfd的读缓冲区
            int i;
            for(i=0; i<1024; ++i)
            {
                if(fds[i].fd == -1)
                {
                    fds[i].fd = connfd;
                    break;
                }
            }
            maxfd = i > maxfd ? i : maxfd;
        }

//        printf("step3\n");

        // 通信, 有客户端发送数据过来
        for(int i=1; i<=maxfd; ++i)
        {
            // 如果在集合中, 说明读缓冲区有数据
            if(fds[i].revents & POLLIN)
            {
                char buf[128];
                int count = read(fds[i].fd, buf, sizeof(buf));
                if(count <= 0)
                {
                    printf("client disconnect ...\n");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    continue;
                }
                else
                {
                    printf("client say: %s\n", buf);
//                    write(fds[i].fd, buf, strlen(buf)+1);
                }

            }

            if(fds[i].fd > -1)
            {
                printf("fds[%d].fd=%d \n",i,fds[i].fd);
                int size = send(fds[i].fd, sendBuf, sendBufLen, 0);
                if (size < 0) {
                    printf("fds[%d].fd=%d,send error \n",i,fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    continue;
                }

            }


        }
    }

    close(serverFd);

    return 0;
}

