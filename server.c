#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <unistd.h> 
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define MESSAGE_SIZE 7
#define PORT 43000
#define BUFFER_SIZE 10

char AA[BUFFER_SIZE*2];
char BB[BUFFER_SIZE*2];
char CC[BUFFER_SIZE*2];
char DD[BUFFER_SIZE*2];

struct Shared
{
  int new; //buf数据是否被reverse读取
  int pid;
  char buf[MESSAGE_SIZE*2];
};
struct Shared* psh = NULL;

// 把buf*2放到对应的AA BB CC DD中
void handleMeaasge(char* buf) {
  char doublebuf[2*BUFFER_SIZE] = {0};
  for(int i=0; i<strlen(buf); i++){
    doublebuf[i] = buf[i];
    doublebuf[i+strlen(buf)] = buf[i];
  }
  doublebuf[strlen(buf)*2-1] = '\0';
  //printf("handle message: %s\n", doublebuf);
  switch (buf[5])
  {
  case 'A':
    strcpy(AA, doublebuf);
    break;
  case 'B':
    strcpy(BB, doublebuf);
    break;
  case 'C':
    strcpy(CC, doublebuf);
    break;
  case 'D':
    strcpy(DD, doublebuf);
    break;
  
  default:
    break;
  }
}

// 接受client发来的数据
void* receiveMessage(void* arg) {
  char buf[BUFFER_SIZE] = {0};
  int cfd = *(int *)arg;
  int recvlen;

  while (1) {
    memset(buf, 0, sizeof(buf));

    while (recv(cfd, buf, MESSAGE_SIZE, 0) > 0){
      //printf("now server%d recv : %s\n", cfd, buf);
      handleMeaasge(buf);  //将收到的数据*2
    }
    printf("recv fail or client exit");
    break;
  }

  close(cfd);
  return NULL;
}

int creatServer() {
  printf("creatServer..\n");
  int serverSock = -1, clientSock = -1;
  struct sockaddr_in serverName;
  struct sockaddr_in clientName;
  socklen_t clientNameLen = sizeof(clientName);
  serverSock = socket(PF_INET, SOCK_STREAM, 0);
  if(serverSock==-1)
    printf("socket failed:%d",errno);
  serverName.sin_family = AF_INET;
  serverName.sin_port = htons(PORT);
  serverName.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(serverSock, (struct sockaddr *)&serverName, sizeof(serverName));
  listen(serverSock, 5);

  //pthread_t tids[NUM_THREADS];
  pthread_t tid;
  while (1) {
    printf("等待连接...\n");
    clientSock = accept(serverSock, (struct sockaddr *)&clientName, &clientNameLen);
    if(clientSock==-1){
      printf("accept failed:%d",errno);
      return -1;
    }
    printf("新的连接%d成功\n", clientSock);
    // 每接受到一个client的连接请求就开一个新的线程来处理发来的数据 i am a/b/c/d
    //参数依次是：创建的线程id，线程参数，调用的函数，传入的函数参数
    int ret = pthread_create(&tid, NULL, receiveMessage, (void*)&(clientSock));
    if (ret != 0) {
      printf("pthread_create error: error_code = %d\n", ret);
      return -1;
    }
    pthread_detach(tid); //线程回收，线程结束之后自动回收
  }

  printf("over server");
  close(serverSock);
  return 0;
}

// 监控ABCD内容，并向共享缓存psh里更新
void* writeShm(void* args) {
  printf("begain writeShm------");
  //int pid = *(int *)args;
  //printf("writeShm pid = %d.", pid);
  psh->new = 0;
  int new;

  // 监控AA BB CC DD中的数据变化，如有新数据，
  // 就写入共享内存psh->buf, 同时置psh->new为1(表示这是一个reverse进程还没读取的新数据)
  while(1) {
    if(AA[0] != 0){
      printf("server write AA:%s to p->buf\n", AA);
      strcpy(psh->buf, AA);
      AA[0] = 0;
      psh->new = 1;
    }
    if(DD[0] != 0){
      printf("server write DD:%s to p->buf\n", DD);
      strcpy(psh->buf, DD);
      DD[0] = 0;
      psh->new = 1;
    }
    if(BB[0] != 0){
      printf("server write BB:%s to p->buf\n", BB);
      strcpy(psh->buf, BB);
      BB[0] = 0;
      psh->new = 1;
    }
    if(CC[0] != 0){
      printf("server write CC:%s to p->buf\n", CC);
      strcpy(psh->buf, CC);
      CC[0] = 0;
      psh->new = 1;
    }
  }
  printf("write is over!!\n");
  return NULL;
}

void myfun(int signum)
{
    return;
}

void ShareMemRemove(struct Shared** p , int shmid)
{
	// 解除映射共享内存
	shmdt(*p);
	// 销毁共享内存
	shmctl(shmid, IPC_RMID, NULL);
}

int main() {
  int shmid;
  int pid; //进程id,本来想用来做signal同步的，
  int key;

  // 创建共享内存
  key = ftok("./Shared", 'a');
  
  shmid = shmget(key, 128, IPC_CREAT | 0777);
    if (shmid < 0) {
        printf("create shared memory fail\n");
        return -1;
    }
  printf("create shared memory sucess, shmid = %d\n", shmid);
  signal(SIGUSR2, myfun);
  psh = (struct Shared *)shmat(shmid, NULL, 0);
  
  if (psh == NULL) {
    printf("shmat fail\n");
    return -1;
  }
  printf("parent process shmat sucess\n");
  // 在共享内存里和reverse进程交换各自id,只是为了让这两个进程大致同时开启，可以省略
  psh->pid = getpid();
  pause();
  pid = psh->pid;
  printf("my server pid = %d and reverse pid = %d.\n", getpid(), pid);
  
  pthread_t id;
  // 开启一个新的线程准备往共享内存里写数据
  int ret = pthread_create(&id, NULL, writeShm, NULL); 

  // 接受client发来的数据，
  creatServer();
  
  //ShareMemRemove((struct Shared **)&psh, shmid);
  //printf("server shoutdown!!\n");
  return 0;
}
