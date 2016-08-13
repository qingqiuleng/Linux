#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>


void usage(char * argv)
{
	printf("%s\n", argv);
}

int startup(char * ip, int port)
{
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket<0)
	{
		perror("socket");
		exit(1);
	}

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);

	//绑定
	if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))<0)
	{
		perror("bind");
		exit(2);
	}

	//监听
	if (listen(listen_socket, 5))
	{
		perror("listen");
		exit(3);
	}

	return listen_socket;
}

int fds[64];
int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		usage(argv[0]);
		exit(1);
	}

	char*ip = argv[1];
	int port = atoi(argv[2]);

	int listen_socket = startup(ip, port);//创建一个描述本地IP和端口号的套接字
	struct sockaddr_in peer;//保存远端的IP和端口信息
	socklen_t len = sizeof(peer);

	int max_fd = -1;//文件描述符最大值
	fd_set writes; //写文件描述符集
	fd_set reads; //读文件描述符集
	int new_socket = -1;

	int fds_num = sizeof (fds) / sizeof(fds[0]);
	int i = 0;

	for (; i<fds_num; i++)
	{
		fds[i] = -1;
	}

	fds[0] = listen_socket; //将listen_socket写入到文件描述符数组中

	int done = 0;
	while (!done)
	{
		//每次循环将读文件和写文件描述集进行初始化
		FD_ZERO(&writes);
		FD_ZERO(&reads);

		struct timeval _timeout = { 5, 0 };

		for (i = 0; i<fds_num; i++)
		{
			if (fds[i]>0)
			{
				FD_SET(fds[i], &reads);//将需要测试的fd添加到fd_set中
				if (fds[i]>max_fd)
				{
					max_fd = fds[i];//获取最大文件描述符
				}
			}
		}

		switch (select(max_fd + 1, &reads, &writes, NULL, &_timeout))
		{
		case 0:
			printf("timeout\n");
			break;
		case -1:
			perror("select");
			break;
		default:
		{
				   for (i = 0; i<fds_num; i++)//轮询查看所有的fd
				   {
					   if (fds[i] = listen_socket&&FD_ISSET(fds[i], &reads))//用于检查某个fd在select后相应的位是否还为1（这里接收的是本地监听套接字）
					   {
						   new_socket = accept(listen_socket, (struct sockaddr*)&peer, &len);
						   if (new_socket<0)
						   {
							   perror("accept");
							   continue;
						   }
						   printf("conect succeed:%d\n", new_socket);//等待接收远端的套接字成功
						   for (i = 0; i<fds_num; i++)
						   {
							   if (fds[i] == -1)
							   {
								   fds[i] = new_socket;//将创建好的远端套接字加入到fd数组中
								   break;
							   }

						   }
					   }
					   else if (fds[i]>0 && FD_ISSET(fds[i], &reads))//监听的是远端套接字，可以进行数据发送和读写了
					   {
						   char buf[1024];
						   ssize_t s = read(fds[i], buf, sizeof (buf)-1);

						   if (s>0)
						   {
							   buf[s] = '\0';
							   printf("%s\n", buf);
						   }
						   else if (s == 0)
						   {
							   fds[i] = -1;
							   printf("client is closed\n");
						   }
						   else
							   perror("read");
					   }
					   else
						   continue;
				   }
		}

		}
	}

	return 0;
}














