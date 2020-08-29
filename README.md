## client-server-reverse-print
Using C language for TCP UDP communication under Linux

### what

Client的4个进程A B C D.周期为100ms、500ms、1000ms、100ms地向server进程发送"I AM ?"(? in [A B C D]).
Server接受多个client的数据，分别存入对应区域AA BB CC DD；另开一个线程把4个区域数据及时更新到共享内存.
Reverse从共享内存读数据，反转，udp发送给printing进程.
Printing接受数据并打印.

### communication
- client to server: tcp
- server and reverse：共享内存、信号量
- reverse to printing：udp
