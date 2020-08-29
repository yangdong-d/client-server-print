#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <errno.h>

#define DEST_IP "127.0.0.1"
#define DEST_PORT 43000

// tcp向127.0.0.1：43000 周期地发送buf
void sendMessage(char cid, int T){
  //建立连接
  int serverSock = -1;  
  struct sockaddr_in destAddr;
  serverSock = socket(AF_INET, SOCK_STREAM, 0);
  if(serverSock==-1)
    printf("socket failed:%d",errno);
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(DEST_PORT);
  destAddr.sin_addr.s_addr = inet_addr(DEST_IP);
  if(connect(serverSock, (struct sockaddr*)&destAddr, sizeof(struct sockaddr))==-1)
    printf("connect failed:%d\n",errno);

  // 按周期T循环发送
  while(1){
    char buf[10] = "I AM ";
    buf[5] = cid;
    buf[6] = ' ';
    buf[7] = '\0';
    printf("client %c即将发送:%s\n", cid, buf);
    
    send(serverSock, buf, strlen(buf), 0);
    usleep(T);
  }
  return;
}

void creatClient(int c){
  char cid = 'A'; //client进程编号ABCD
  int T;          //发送周期

  switch(c){
    case 0:
      T = 100;
      break;
    case 1:
      T = 500;
      break;
    case 2:
      T = 1000;
      break;
    case 3:
      T = 100;
      break;
  }
  // 设定各个client的发送字符和周期
  cid += c;
  T *= 1000;

  // 创建子进程
  int cpid = fork();  
    if (cpid < 0)  
      printf("fork failed\n");  
    else if (cpid == 0) // child process  
      sendMessage(cid, T);
    else
      return;
}

int main(){
  // 创建ABCD4个client子进程
  for (int i=0; i<4; i++){
    creatClient(i);
    //加点延迟
    usleep(137*1000);
  }
  return 0;
}

