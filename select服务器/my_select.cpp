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

	//��
	if (bind(listen_socket, (struct sockaddr*)&local, sizeof(local))<0)
	{
		perror("bind");
		exit(2);
	}

	//����
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

	int listen_socket = startup(ip, port);//����һ����������IP�Ͷ˿ںŵ��׽���
	struct sockaddr_in peer;//����Զ�˵�IP�Ͷ˿���Ϣ
	socklen_t len = sizeof(peer);

	int max_fd = -1;//�ļ����������ֵ
	fd_set writes; //д�ļ���������
	fd_set reads; //���ļ���������
	int new_socket = -1;

	int fds_num = sizeof (fds) / sizeof(fds[0]);
	int i = 0;

	for (; i<fds_num; i++)
	{
		fds[i] = -1;
	}

	fds[0] = listen_socket; //��listen_socketд�뵽�ļ�������������

	int done = 0;
	while (!done)
	{
		//ÿ��ѭ�������ļ���д�ļ����������г�ʼ��
		FD_ZERO(&writes);
		FD_ZERO(&reads);

		struct timeval _timeout = { 5, 0 };

		for (i = 0; i<fds_num; i++)
		{
			if (fds[i]>0)
			{
				FD_SET(fds[i], &reads);//����Ҫ���Ե�fd��ӵ�fd_set��
				if (fds[i]>max_fd)
				{
					max_fd = fds[i];//��ȡ����ļ�������
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
				   for (i = 0; i<fds_num; i++)//��ѯ�鿴���е�fd
				   {
					   if (fds[i] = listen_socket&&FD_ISSET(fds[i], &reads))//���ڼ��ĳ��fd��select����Ӧ��λ�Ƿ�Ϊ1��������յ��Ǳ��ؼ����׽��֣�
					   {
						   new_socket = accept(listen_socket, (struct sockaddr*)&peer, &len);
						   if (new_socket<0)
						   {
							   perror("accept");
							   continue;
						   }
						   printf("conect succeed:%d\n", new_socket);//�ȴ�����Զ�˵��׽��ֳɹ�
						   for (i = 0; i<fds_num; i++)
						   {
							   if (fds[i] == -1)
							   {
								   fds[i] = new_socket;//�������õ�Զ���׽��ּ��뵽fd������
								   break;
							   }

						   }
					   }
					   else if (fds[i]>0 && FD_ISSET(fds[i], &reads))//��������Զ���׽��֣����Խ������ݷ��ͺͶ�д��
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














