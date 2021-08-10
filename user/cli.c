/*
 * @Author: your name
 * @Date: 2021-08-09 15:12:24
 * @LastEditTime: 2021-08-09 15:13:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \personnele:\华清远见\share\user\cli.c
 */
#include<stdio.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<fcntl.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<dirent.h>
#include<stdlib.h>
#include<sqlite3.h>
#include<arpa/inet.h>
#include<signal.h>
#include<pthread.h>

#define ERR_LOG(errmsg) do{\
	perror(errmsg);\
	fprintf(stderr,"%s %s %d\n",__FILE__,__func__,__LINE__);\
}while(0)

#define N 128
#define IP "0.0.0.0"
#define PORT 8888

#define REG 'R'//注册指令
#define LOGIN 'L' //登录指令
#define QUERY 'Q' //查询单词
#define HISTORY 'H' //查询历史
#define EXIT 'E' //退出

typedef struct{
	char use[10];//保存账号
	char password[10];//保存密码
	char username[10];//保存用户名
	int age;//保存年龄
	char sex[10];//保存性别
	char address[20];//保存地址
	int pthone;//保存手机号
	int level;//保存员工等级
	int money;//保存工资
	int state; //用户状态，0:未登录 1:已登录
}MSG;	//用户表信息

typedef struct{
	char use[10];//保存账号
	char password[10];//保存密码
	char username[10];//保存用户名
	int pthone;//保存手机号
	int key;//注册码
	int state;//用户状态
}ROOT;	//管理员表信息

int up_ser(int sfd);
int do_REG(int sfd);
int do_user(int sfd);
int do_root(int sfd);
int do_END(int sfd);
int do_LOGIN(int sfd);
int do_userlogin(int sfd);
int do_rootlogin(int sfd);
int do_up_login(int sfd);
int do_query(int sfd);
int do_up_root(int sfd);
int do_query_all(int sfd);
//-----------------------------------------------主函数
int main(int argc,const char *argv[])
{
	char *errmsg=NULL;

	//连接服务器
	int sfd=up_ser(sfd);
	if(sfd<0)
	{
		ERR_LOG("socket");
		printf("连接失败\n");
		return -1;
	}



	int choose=0;
	while(1)
	{

		system("clear");
		choose = 0;

		printf(" =================================================== \n");
		printf("|   广告位招租                                      |\n");
		printf("|    联系作者                                       |\n");
		printf(" =================================================== \n");
		printf("|     ___________________                           |\n");
		printf("|    |                   |                          |\n");
		printf("|    |                   |                          |\n");
		printf("|                        |                          |\n");
		printf("|     ___         ___    |      卓远软件 (汉化版)   |\n");
		printf("|    |   |   |   |   |   |                          |\n");
		printf("|    |   |   |   |   |   |                          |\n");
		printf("|    |   |___|   |   |___|                          |\n");
		printf("|    |                                              |\n");
		printf("|    |                                              |\n");
		printf("|    |                   |                          |\n");
		printf("|    |___________________|         作者:champion    |\n");
		printf("|                                                   |\n");
		printf(" =================================================== \n");
		printf("|请选择:                                            |\n");
		printf("|         1.注册     2.登录      3.退出             |\n");
		printf(" =================================================== \n");
		printf("序号>>");

		scanf("%d",&choose);
		while(getchar()!=10);
		switch(choose)
		{  
		case 1:
			do_REG(sfd);//注册
			break;
		case 2:
			do_LOGIN(sfd);//登录
			break;
		case 3:
			goto END;//退出
			break;

		default:
			printf("输入错误\n");
		}


		printf("按任意键清屏>>>");
		while(getchar()!=10);

	}
END:
	close(sfd);
	return 0;
}

//--------------------------------------------主函数结尾

//连接服务器
int up_ser(int sfd)
{
	//{{{
	//SOCKET
	sfd=socket(AF_INET,SOCK_STREAM,0);
	if(sfd<0)
	{
		ERR_LOG("socket");
		return -1;
	}

	//端口快速重用
	int reuse=1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))<0)
	{
		ERR_LOG("setsockopt");
		return -1;
	}

	//填充服务器
	struct sockaddr_in sin;
	sin.sin_family=AF_INET;
	sin.sin_port=htons(PORT);
	sin.sin_addr.s_addr=inet_addr(IP);

	if(connect(sfd,(struct sockaddr *)&sin,sizeof(sin))<0)
	{
		ERR_LOG("connect");
		return -1;
	}

	printf("连接服务器成功\n");

	return sfd;
	//}}}
}

//注册
int do_REG(int sfd)
{
	//{{{
	
	char buf[N]="";
	bzero(buf,sizeof(buf));
	buf[0]=REG;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}


	int choose=0;
	while(1)
	{
		system("clear");
		choose = 0;
		printf("-------------------------\n");
		printf("---- 1.注册普通用户 -----\n");
		printf("---- 2.注册管理用户 -----\n");
		printf("---- 3.返回上一级   -----\n");
		printf("-------------------------\n");

		printf("请输入>>>");
		scanf("%d",&choose);
		while(getchar()!=10);

		switch(choose)
		{
		case 1:
			do_user(sfd);//注册普通用户
			break;
		case 2:
			do_root(sfd);//注册管理用户
			break;
		case 3:
			do_END(sfd);//退出
			return -1;

		default:
			printf("输入错误\n");
		}


		printf("按任意键清屏>>>");
		while(getchar()!=10);

	}
	return 0;
	//}}}
}

//注册普通用户
int do_user(int sfd)
{
	//{{{
	char buf[N]="";
	int res;
	MSG msg;
	char buff[N]="";

	bzero(buf,sizeof(buf));
	buf[0]=1;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
	printf("您需要输入一下信息>> 账号，密码，姓名，年龄，性别，地址，手机号，员工等级，工资\n");
	printf("输入新账号>>");
	scanf("%s",msg.use);
	while(getchar()!=10);

	printf("输入新密码>>");
	scanf("%s",msg.password);
	while(getchar()!=10);

	printf("输入姓名>>");
	scanf("%s",msg.username);
	while(getchar()!=10);

	printf("输入年龄>>");
	scanf("%d",&msg.age);
	while(getchar()!=10);


	printf("输入性别(男:GG/女:MM)>>");
	scanf("%s",msg.sex);
	while(getchar()!=10);


	printf("输入地址>>");
	scanf("%s",msg.address);
	while(getchar()!=10);

	printf("输入手机号>>");
	scanf("%d",&msg.pthone);
	while(getchar()!=10);

	printf("输入员工等级(由低到高:1~7)>>");
	scanf("%d",&msg.level);
	while(getchar()!=10);

	printf("输入工资>>");
	scanf("%d",&msg.money);
	while(getchar()!=10);

	msg.state=0;
	res=send(sfd,&msg,sizeof(msg),0);
	if(res<0)
	{
		ERR_LOG("recv");
		return -1;
	}

	//读取服务器的反馈
	recv(sfd,buff,sizeof(buff),0);
	printf("%s\n",buff);	


	return 0;
	//}}}
}

//注册管理用户
int do_root(int sfd)
{


	//{{{
	char buf[N]="";
	int res;
	ROOT msg;
	char buff[N]="";

	bzero(buf,sizeof(buf));
	buf[0]=2;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
	printf("您需要输入一下信息>> 账号，密码，姓名，手机号，注册码(重要)\n");
	printf("输入新账号>>");
	scanf("%s",msg.use);
	while(getchar()!=10);


	printf("输入新密码>>");
	scanf("%s",msg.password);
	while(getchar()!=10);

	printf("输入姓名>>");
	scanf("%s",msg.username);
	while(getchar()!=10);

	printf("输入手机号>>");
	scanf("%d",&msg.pthone);
	while(getchar()!=10);

	printf("输入注册码>>");
	scanf("%d",&msg.key);
	while(getchar()!=10);


	msg.state=0;
	res=send(sfd,&msg,sizeof(msg),0);
	if(res<0)
	{
		ERR_LOG("recv");
		return -1;
	}

	//读取服务器的反馈
	recv(sfd,buff,sizeof(buff),0);
	printf("%s\n",buff);	

	return 0;
	//}}}
}

//返回上一级
int do_END(int sfd)
{
	
	//{{{
	char buf[N]="";
	bzero(buf,sizeof(buf));
	buf[0]=0;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}

	return -1;
	//}}}
}

//登录
int do_LOGIN(int sfd)
{
	//{{{
	char buf[N]="";
	bzero(buf,sizeof(buf));
	buf[0]=LOGIN;
	if(send(sfd,buf,sizeof(buf),0)<0)   //发送协议
	{
		ERR_LOG("send");
		return -1;
	}

	int choose=0;
	while(1)
	{
		system("clear");
		choose = 0;
		printf("-------------------------\n");
		printf("---- 1.登录普通用户 -----\n");
		printf("---- 2.登录管理用户 -----\n");
		printf("---- 3.返回上一级   -----\n");
		printf("-------------------------\n");

		printf("请输入>>>");
		scanf("%d",&choose);
		while(getchar()!=10);

		switch(choose)
		{
		case 1:
			if(5==do_userlogin(sfd))//登录普通用户
			{
				do_up_login(sfd);//登录后的功能
			}
			break;
		case 2:
			if(5==do_rootlogin(sfd))//登录管理用户
			{
				do_up_root(sfd);//登陆后的功能
			}
			break;
		case 3:
			do_END(sfd);//退出
			return -1;

		default:
			printf("输入错误\n");
		}


		printf("按任意键清屏>>>");
		while(getchar()!=10);
	}
	return 0;
	//}}}
}


//登录普通用户
int do_userlogin(int sfd)
{
	//{{{
	
	char buf[N]="";
	int res;
	MSG msg;
	char buff[N]="";

	bzero(buf,sizeof(buf));
	buf[0]=1;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
//-------------------------------------------------------------
//	MSG msg;

	printf("输入账号>>");
	scanf("%s",msg.use);
	while(getchar()!=10);


	printf("输入密码>>");
	scanf("%s",msg.password);
	while(getchar()!=10);

//	int res;
	res=send(sfd,&msg,sizeof(msg),0);//发送信息给服务器
	if(res<0)
	{
		ERR_LOG("recv");
		return -1;
	}

//	char buff[N]="";
	recv(sfd,buff,sizeof(buff),0);//读取服务器反馈
	printf("%s\n",buff);
	if(strcmp(buff,"登录成功")==0)
	{
		return 5;
	}

	
	return 0;
	//}}}
}

//管理员登录
int do_rootlogin(int sfd)
{
	//{{{
	
	char buf[N]="";
	int res;
	ROOT msg;
	char buff[N]="";

	bzero(buf,sizeof(buf));
	buf[0]=2;
	if(send(sfd,buf,sizeof(buf),0)<0) //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
//-------------------------------------------------------------
	
	printf("输入账号>>");
	scanf("%s",msg.use);
	while(getchar()!=10);


	printf("输入密码>>");
	scanf("%s",msg.password);
	while(getchar()!=10);

	res=send(sfd,&msg,sizeof(msg),0);//发送信息给服务器
	if(res<0)
	{
		ERR_LOG("recv");
		return -1;
	}

	recv(sfd,buff,sizeof(buff),0);//读取服务器反馈
	printf("%s\n",buff);
	if(strcmp(buff,"登录成功")==0)
	{
		return 5;
	}
	
	return 0;
	//}}}
}

//普通管理
int do_up_login(int sfd)
{
	//{{{
	int choose=0;
	char buf[N]="";
	while(1)
	{
		system("clear");
		choose=0;

		printf("---------------------\n");
		printf("---- 1.查询信息 -----\n");
		printf("---- 2.退出登录 -----\n");
		printf("---------------------\n");

		printf("请输入>>>");
		scanf("%d",&choose);
		while(getchar()!=10);


		switch(choose)
		{
		case 1:
			//查信息
			do_query(sfd);
			break;
		case 2:
			//退出

			bzero(buf,sizeof(buf));
			buf[0]=EXIT;      
			if(send(sfd,buf,sizeof(buf),0)<0)   //发送协议
			{
				ERR_LOG("send");
				return -1;
			}
			printf("退出登录成功\n");
			return -1;
		default :
			printf("输入错误\n");


		}

		printf("按任意键清屏>>>");
		while(getchar()!=10);

	}
	
	
	return 0;
	//}}}
}

//查信息
int do_query(int sfd)
{
	//{{{
	char buf[N]="";
	bzero(buf,sizeof(buf));
	buf[0]=QUERY;      
	if(send(sfd,buf,sizeof(buf),0)<0)   //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
	//-----------------------------------------------

	//输入要查询的单词
	printf("输入要查询的账号>>");
	char english[N]="";
	bzero(english,sizeof(english));
	fgets(english,N,stdin);
	english[strlen(english)-1]=0;

	if(send(sfd,english,sizeof(english),0)<0)
	{
		ERR_LOG("send");
		return -1;
	}


	char chinese[N]="";
	int i;
	int j;
	while(1)
	{
		bzero(chinese,sizeof(chinese));
		recv(sfd,chinese,N,0); //接受反馈
		i=strcmp(chinese,"查询完毕");
		if(i==0)
		{
			printf("查询完毕\n");
			break;
		}
		else 
		{
			j=strcmp(chinese,"抱歉，该用户没有信息");
			if(j==0)
			{
				printf("抱歉，该用户没有信息\n");
				break;
			}
			else
				printf("%s\n",chinese);

		}
		printf("查询完毕\n");
		return 0;
	}	
	
	
	return 0;
	//}}}
}


//管理员模式
int do_up_root(int sfd)
{
	//{{{
	
	int choose=0;
	char buf[N]="";
	while(1)
	{
		system("clear");
		choose=0;

		printf("---------------------\n");
		printf("---- 1.查询所有信息 -\n");
		printf("---- 2.修改信息 -----\n");
		printf("---- 3.删除信息 -----\n");
		printf("---- 4.退出登录 -----\n");
		printf("---------------------\n");

		printf("请输入>>>");
		scanf("%d",&choose);
		while(getchar()!=10);


		switch(choose)
		{
		case 1:
			//查信息
			do_query_all(sfd);
			break;
		case 2:
			//退出

			bzero(buf,sizeof(buf));
			buf[0]=EXIT;      
			if(send(sfd,buf,sizeof(buf),0)<0)   //发送协议
			{
				ERR_LOG("send");
				return -1;
			}
			printf("退出登录成功\n");
			return -1;
		default :
			printf("输入错误\n");


		}

		printf("按任意键清屏>>>");
		while(getchar()!=10);

	}
	
	
	
	return 0;
	//}}}
}

//查询所有信息
int do_query_all(int sfd)
{
	//{{{
	char buf[N]="";
	bzero(buf,sizeof(buf));
	buf[0]=QUERY;      
	if(send(sfd,buf,sizeof(buf),0)<0)   //发送协议
	{
		ERR_LOG("send");
		return -1;
	}
	//-----------------------------------------------
	
	char chinese[4096]="";
	int i;
	int j;
	while(1)
	{
		bzero(chinese,sizeof(chinese));
		recv(sfd,chinese,N,0); //接受反馈
		i=strcmp(chinese,"查询完毕");
		if(i==0)
		{
			printf("查询完毕\n");
			return -1;
		}
		else 
		{
			j=strcmp(chinese,"抱歉，该用户没有信息");
			if(j==0)
			{
				printf("抱歉，该用户没有信息\n");
				break;
			}
			else
				printf("%s\n",chinese);

		}
	}
	return 0;
	//}}}
}
