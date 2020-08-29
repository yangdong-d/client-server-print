#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
    
#define PATH "./pri"  
#define BUFFER_SIZE 20

int main(){
    int sockfd = 0;  
    struct sockaddr_un addr;  
    unlink(PATH);    
    addr.sun_family = AF_UNIX;  
    strcpy(addr.sun_path,PATH);  
    
    unsigned int len = strlen(addr.sun_path) + sizeof(addr.sun_family);  
    sockfd = socket(AF_UNIX,SOCK_DGRAM,0);  

    if(sockfd < 0 )  
    {  
        perror("socket error");  
        exit(-1);  
    }  
    
    if(bind(sockfd,(struct sockaddr *)&addr,len) < 0)  
    {  
        perror("bind error");  
        close(sockfd);  
        exit(-1);  
    }  
    printf("Bind is ok\n");  
    
    // 接受reverse进程发来（udp）的数据 并打印到控制台
    while(1)  
    {  
        char recv_buf[BUFFER_SIZE] = "";  
        recvfrom(sockfd,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&addr,&len);  
        printf("printing Recv: %s\n",recv_buf);  
    }  

    return 0;
}