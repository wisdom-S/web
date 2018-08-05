#include"web_htpp.h"

int main(int argc,char** argv)
{
    if(argc != 3)
        printf("please input server_addr![IP][port]\n");

    int listener = init_listen(argv[1],atoi(argv[2]));

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int sockfd = accept(listener,(struct sockaddr*)&client_addr,&len);
        if(sockfd < 0)
        {
            printlog("accept fali!",FATAL);
            continue;
        }

        pthread_t pid;
        pthread_create(&pid,NULL,accept_req,(void*)sockfd);
    }

    close(listener);
    return 0;
}

int init_listen(const char* ip, int port)
{
    assert(ip);

    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    {
         printlog("create socket failed!",FATAL);
         return 1;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in ser_addr;
    ser_addr.sin_family = AF_INET;
    if(inet_aton(ip, &ser_addr.sin_addr) == 0)
    {
        printlog("inet_aton fail!",FATAL);
        return 1;
    }
    ser_addr.sin_port = htons(port);
    socklen_t len = sizeof(ser_addr);

    if(bind(sock,(struct sockaddr*)&ser_addr,len) <0 )
    {
        printlog("bind failed!",FATAL);
        return 2;
    }

    if(listen(sock,10) < 0)
    {
        printlog("listen fail!",FATAL);
        return 3;
    }

    return sock;
}

void* accept_req(void* arg)
{
    int sock =(int)arg;
    pthread_detach(pthread_self());
    return (void*) handle(sock);
}

