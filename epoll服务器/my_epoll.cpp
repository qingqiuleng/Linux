#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<unistd.h>
#include<string.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<arpa/inet.h>

void usage(char* argv)
{
	printf("usage:[ip] [port]\n", argv);
}

int startup(const char*_ip, int _port)//创建本地套接字
{
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket<0)
	{
		perror("socket");
		exit(2);
	}

	int opt = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//防止client断开后还继续占用                                   server一段时间

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(_port);
	local.sin_addr.s_addr = inet_addr(_ip);

	if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))<0)
	{
		perror("bind");
		exit(3);
	}

	if (listen(listen_socket, 5)<0)
	{
		perror("listen");
		exit(4);
	}

	return listen_socket;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		usage(argv[1]);
		exit(1);
	}

	int listen_sock = startup(argv[1], atoi(argv[2]));

	int epfd = epoll_create(256);//创建一个epoll句柄
	if (epfd<0)
	{
		perror("epoll_create");
		exit(2);
	}

	struct epoll_event _ev;//需要监听的事件
	_ev.events = EPOLLIN;
	_ev.data.fd = listen_sock;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &_ev);//先注册要监听事件的类型（第二参数表示对fd的动作），_ev即是所要监听的的事件

	struct epoll_event revs[64];

	int timeout = -1;
	int num = 0;
	int done = 0;

	while (!done)
	{
		switch ((num = epoll_wait(epfd, revs, 64, timeout)))//将监听发生过的事件赋值到revs中
		{
		case 0:
			printf("timeout\n");
			break;
		case -1:
			printf("epoll_wait\n");
			break;
		default://就绪文件描述符的值，通过映射找到相关的文件描述符
		{
					struct sockaddr_in peer;
					socklen_t len = sizeof(peer);
					int i = 0;
					for (; i<num; i++)//遍历的是已经等待成功的事件的数目（和select的区别，select是遍历自己定义的整个的文件描述符的数组）
					{
						int rsock = revs[i].data.fd;
						if (rsock == listen_sock&&\
							(revs[i].events&EPOLLIN))
						{
							int new_fd = accept(listen_sock, (struct sockaddr*)&peer, &len);//接收远端的套接字
							if (new_fd>0)
							{
								printf("get a new client:%s:%s\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
								//            set_nonblock(new_fd);
								_ev.events = EPOLLIN | EPOLLET;
								_ev.data.fd = new_fd;
								epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &_ev);
							}
						}
						else//远端的套接字已经获取成功，可以进行数据的发送和读写了
						{
							if (revs[i].events&EPOLLIN)
							{
								char buf[1024];
								ssize_t s = read(rsock, buf, sizeof(buf));
								if (s>0)
								{
									buf[s] = '\0';
									printf("client:%s\n", buf);

									_ev.events = EPOLLOUT | EPOLLET;
									_ev.data.fd = rsock;
									epoll_ctl(epfd, EPOLL_CTL_MOD, rsock, &_ev);
								}
								else if (s == 0)
								{
									printf("client is closed...:%d\n", rsock);
									epoll_ctl(epfd, EPOLL_CTL_DEL, rsock, NULL);
									close(rsock);
								}
								else
								{
									perror("read");
								}

							}
							else if (revs[i].events&EPOLLOUT)
							{
								const char* msg = "HTTP/1.0 200 OK\r\n\r\n<html><h1>hello bvit!</h1></html>\r\n";
								write(rsock, msg, strlen(msg));
								epoll_ctl(epfd, EPOLL_CTL_DEL, rsock, NULL);
								close(rsock);
							}

						}

					}
		}
		}
	}
}












