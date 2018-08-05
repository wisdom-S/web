#include"web_http.h"

void printlog(const char* log,int level)
{
    char* err_list[5]={
        "NORMAL",
        "WARNING",
        "FATAL"
    };

    char* log_path = "log/filelog";
    int fd = open(log_path,O_WRONLY | O_APPEND | O_CREAT, 0644);

    char buff[SIZE];
    memset(buff,0,sizeof(buff));
    strncpy(buff,err_list[level],strlen(err_list[level]));
    strncat(buff,": ",strlen(": "));
    strncat(buff,log,strlen(log));

    time_t t;
    char ti[30];
    ti[0] = '';
    time(&t);
    strncpy(ti+1,ctime(&t));
    strncat(buf,ti);
    write(fd,buf,,sizeof(buf));
    close(fd);
}

int handle(int sock)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int cgi = 0;
    char buf[SIZE];
    char method[64];
    char url[SIZE];
    char path[SIZE];
    char* query_str = NULL;
    if(getline(sock,buf,sizeof(buf)) <= 0)
    {
        printlog("read request failed!",FATAL);
        echo_errno(sock,404);
        ret = 5;
        goto end;
    }

    while(i<sizeof(buf)-1 && j<sizeof(method)-1 && buf[i] != ' ')
        method[j++] = buf[i++];
    method[j] = '\0';

    if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
    {
        echo_errno(sock,404);
        ret = 6;
        goto end;
    }

    if(strcasecmp(method,"POST") == 0)
    {
        cgi = 1;
    }

    while(i<sizeof(buf)-1 && buf[i] == ' ')
        i++;
    
    j = 0;
    while(i<sizeof(buf)-1 && j<sizeof(url)-1 && buf[i] != ' ')
        buf[j++] = buf[i++];
    buf[j] = '\0';

    if(strcasecmp("GET",method) == 0)
    {
        query_str = url;
        while(*query_str != '\0' && *query_str != '?')
            query_str++;

        if(*query_str == '?')
        {
            *query_str ='\0';
            query_str++;
            cgi = 1;
        }

    }

    sprintf(path,"wwwroot%s",url);
    if(path[strlen[path]-1] == '/')
        strcat(path,"index.html");

    struct stat st;
    if(stat(path,&st) <0)
    {
        printlog("file is not exist! ",WARNING);
        clear_header(sock);
        echo_errno(sock,404);
        ret = 7;
        goto end;
    }
    else
    {
        if(S_ISREG(st.st_mode))
        {
            if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
                cgi = 1;
        }
        else if(S_ISDIR(st.st_mode))
            strcat(path, "/index.html");
        else
        {

        }
    }

    if(cgi == 1)
    {
        ret = cgi_run(sock,method,path,query_str);
    }
    else
    {
        ret = clear_header(sock);
        ret = echo_html(sock,path,st.st_size);
    }

end:
    close(sock);
    return ret;
}

int getline(int sock, char* buf, int len)
{
    assert(buf);
    char ch = '\0';
    int i = 0;
    while(i<len-1 && ch != '\n')
    {
        if(recv(sock,&ch,1,0) > 0)
        {
            if(ch == '\r')
            {
                if(recv(sock,&ch,1,MSG_PEEK)>0 && ch == '\n')
                    recv(sock,&ch,1,0);
                else
                    ch = '\n';
            }
            buf[i++] = ch;
        }
    }
     buf[i] = '/0';
     return i;
}

void echo_errno(int sock,int code)
{
    switch(code)
    {
        case 404:
            err_request("wwwroot/404.html","HTTP/1.0 404 NOT Found\r\n",sock);
            break;
        case 503:
            err_request("wwwroot/503.html","HTTP/1.0 503 Server Unavailable\r\n",sock);
            break;
        default:
            break;
    }
}

void err_request(const char* path,const char* state,int sock)
{
    struct stat st;
    if(stat(path,&st) < 0)
    {
        return;
    }
    int fd = open(path, O_RDONLY);

    const char* status = state;
    send(sock, status,strlen(status),0);
    const char* content_type = "Content-Type:text/html;charset = ISO-8859-1\r\n";
    send(sock,content_type,strlen(content_type),0);
    send(sock,"\r\n",2,0);
    sendfile(sock,fd,NULL,st.st_size);
    close(fd);
}

int clear_hander(int sock)
{
    char line[SIZE];
    int ret = -1;
    do
    {
        ret = getline(sock,line,SIZE);
    }
    while(ret != 1 && strcmp(line,"\n") != 0);

    return ret;
}

static int echo_http(int sock, const char* path, size_t size)
{
    int ret = -1;
    int fd = open(path,O_RDONLY);
    if(fd < 0)
    {
        printlog("open file failed!",FATAL);
        echo_errno(sock,404);
        ret = 8;
    }

    char head[SIZE*10];
    sprintf(head,"HTTP/1.0 200 OK\r\n");
    send(sock,head,strlen(head),0);
    send(sock,"\r\n",2,0);
    if(sendfile(sock,fd,NULL,size) < 0)
    {
        printlog("send html failed ",FATAL);
        echo_errno(sock,503);
        ret = 9;
    }
    close(fd);
    return ret;
}

static int cgi_run(int sock, const char* method, char* path, const char* query_string)
{
    int ret = -1;
    int content_len = -1;
    if(strcasecmp(method, "GET") == 0)
    {
        clear_header(sock);
    }
    else
    {
        char line[SIZE];
        do
        {
            ret = getline(sock,line,sizeof(line));

            if(ret > 0 && strncasecmp(line,"Content-Length: ",16) == 0)
            {
                content_len = atoi(line+16);
            }
        }
        while(ret != 1 && strcmp(line,"\n") != 0);

        if(content_len <0 )
        {
            printlog("haven't content-length arguments! ",FATAL);
            echo_errno(sock,404);
            return 10;
        }
   
    }
    const char* status_head = "HTTP/1.0 200 OK\r\n";
    send(sock,status_head,strlen(status_head),0);
    const char* Content-Type = "Content-Type:text/html;charset = ISO-8859-1\r\n";
    send(sock,Content-Type,strlen(Content-Type),0);
    send(sock,"\r\n",2,0);

    int input[2];
    int ouput[2];
    pipe(input);
    pipe(output);

    pid_t pid;
    pid = fork();
    if(pid == 0)
    {
        close(input[1]);
        close(input[0]);
        dup2(input[0],0);
        dup2(output[1],1);

        char env_method[SIZE];
        char env_query_string[SIZE];
        char env_content_len[SIZE];
        sprintf(env_method,"METHOD=%s",method);
        putenv(env_method);
        if(strcasecmp(method == "GET") == 0)
        {
            sprintf(env_query_string,"QUERY-STRING=%s",query_string);
            putenv(env_query_string);
        }
        else
        {
            sprintf(env_content_len,"CONTENT-LENGTH=%s",content_len);
            putenv(env_content_len);
        }

        execl(path,path,NULL);
        exit(1);
    }
    else
    {
        close(input[0]);
        close(output[1]);

        char c = '\0';
        if(strcasecmp(method,"POST") == 0)
        {
            for(int i = 0;i<content_len;i++)
            {
                recv(sock,&c,1,0);
                write(input[1],&c,1);
            }
        }

        while(read(output[0],&c,1) > 0)
        {
            send(sock,&c,1,0);
        }

        close(input[1]);
        close(output[0]);
        waitpid(pid,NULL,0);
    }
}
