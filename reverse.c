#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/un.h>  

#define PATH "./pri" 

struct Shared
{
  int new;
  int pid;
  char buf[14];
};

void myfun(int signum)
{
  return;
}

void doReverse(char** ss){
  int len = strlen(*ss);
  for (int i=0; i<len/2+1; i++){
    char c = (*ss)[i]; 
    (*ss)[i] = (*ss)[len-i-1];
    (*ss)[len-i-1] = c;
  }
  return;
}

void sendMessage(char* buf){
  int sockfd = 0;  
  struct sockaddr_un addr;  
  bzero(&addr,sizeof(addr));  

  addr.sun_family = AF_UNIX;  
  strcpy(addr.sun_path,PATH);  

  sockfd = socket(AF_UNIX,SOCK_DGRAM,0);  
  if(sockfd < 0)  
  {  
      perror("socket error");  
      exit(-1);  
  }  

  int len = strlen(addr.sun_path)+sizeof(addr.sun_family);  
  sendto(sockfd,buf,strlen(buf),0,(struct sockaddr*)&addr,len);  
  printf("reverse Send: %s\n", buf);  
}

void ShareMemRemove(struct Shared** p , int shmid)
{
	// 解除映射共享内存
	shmdt(*p);
	// 销毁共享内存
	shmctl(shmid, IPC_RMID, NULL);
}

int main(){
  int shmid;
  int key;
  struct Shared *p;
  int pid;
  char* text;

  // 同样的，开一个和server进程一样的共享内存，如果存在，就是同一个
  key = ftok("./Shared", 'a');
  shmid = shmget(key, 128, IPC_CREAT | 0777);
    if (shmid < 0) {
        printf("create shared memory fail\n");
        return -1;
    }
  printf("create shared memory sucess, shmid = %d\n", shmid);
  signal(SIGUSR1, myfun);
  p = (struct Shared *)shmat(shmid, NULL, 0);
      if (p == NULL) {
        printf("shmat fail\n");
        return -1;
      }
  printf("reverse process shmat sucess and pid = %d.\n", getpid());

  // get server pid
  pid = p->pid;
  // tell server this pid
  p->pid = getpid();
  // printf("server pid=%d and reverse pid= %d.", pid, p->pid);
  kill(pid, SIGUSR2);

  int new; // 是否新数据
  char* buf;  // 存储读自共享内存p的字符串
  while(1) {
      // 读new, 如果new==0，则是旧数据，不去读p->buf，继续等
      new = p->new;
      if(new == 0){
        continue;
      }
        
      buf = p->buf;
      // 反转buf
      doReverse(&buf);
      //printf("r: %s\n", buf);
      // 使用udp 发送给printing进程
      sendMessage(buf);
      // 变更new标记，读过之后就是一个旧数据了
      p->new = 0;
  }
  
  // remove 共享内存
  ShareMemRemove(&p, shmid);
  return 0;
}