#include<stdio.h>
#include<wait.h>
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
#include<time.h>

#define ERR_LOG(errmsg) do{\
	perror(errmsg);\
	fprintf(stderr,"%s %s %d\n",__FILE__,__func__,__LINE__);\
}while(0)

#define N 128
#define IP "0.0.0.0"
#define PORT 8888

typedef void (*sighandler_t)(int);
#define REG 'R'  //注册指令
#define LOGIN 'L' //登录指令
#define QUERY 'Q' //查询单词
#define HISTORY 'H' //查询历史
#define EXIT 'E' //退出


int up_sqlite3(sqlite3 *);
int ser(int sfd);
int do_REG(int newfd,sqlite3 *);
int do_user(int newfd,sqlite3 *);
int do_root(int newfd,sqlite3 *);
void *callBackHandler(void *arg);

//线程需要用到
typedef struct{         
	int newfd;   
	struct sockaddr_in cin;
	sqlite3 *db;

}__msgInfo;

typedef struct{
	char use[10];//保存账号
	char password[10];//保存密码
	char username[10];//保存姓名
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
	char username[10];//保存姓名
	int pthone;//保存手机号
	int key;//注册码
	int state; //用户状态，0:未登录 1:已登录
}ROOT;	//管理员表信息

void handler(int sig)//回收子进程资源
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


//---------------------------------------------主函数
int main(int argc, const char *argv[])
{
	//注册信号处理函数
	sighandler_t s= signal(SIGCHLD, handler);
	if(s == SIG_ERR)
	{
		ERR_LOG("signal");
		return -1;
	}
	char *errmsg=NULL;

	//初始化数据库
	sqlite3 *db=NULL;
	up_sqlite3(db);


	//创建服务器
	int sfd=ser(sfd);

	//获取新的文件描述符
	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);

	while(1)
	{
		int newfd = accept(sfd, (struct sockaddr*)&cin, &addrlen);

		if(newfd < 0)
		{
			ERR_LOG("accept");
			return -1;
		}
		printf("[%s:%d]链接成功\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));


		//开线程
		__msgInfo mmm;
		mmm.newfd=newfd;

		pthread_t tid;
		if(pthread_create(&tid,NULL,callBackHandler,(void*)&mmm)!=0)
		{
			ERR_LOG("pthread_create");
			return -1;
		}



	}
	return 0;
}

//------------------------------------------------------------主函数结尾

//导入数据库
int up_sqlite3(sqlite3 *db)
{
	//{{{
	//创建并打开数据库
	if(sqlite3_open("./personnel.db",&db)!=0)
	{
		printf("sqlite3_open 失败\n");
		printf("sqlite3_open:%s\n",sqlite3_errmsg(db));
		return -1;
	}

	//创建表格
	char sql[256]="";
	char sqll[128]="";
	char *errmsg;
	//创建普通用户表
	sprintf(sql,"create table if not exists user(use char primary key,password char,name char,age int,sex char,address char,pthone int,level char,money int,state int)");
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:__%d__ %s\n",__LINE__,errmsg);
		return -1;
	}
	//创建管理员表
	sprintf(sqll,"create table if not exists root(use char primary key,password char,name char,pthone int,state int)");
	if(sqlite3_exec(db,sqll,NULL,NULL,&errmsg)!=0)
	{
		printf("sqlite3_exec:__%d__ %s\n",__LINE__,errmsg);
		return -1;
	}
	//-----------------------------------
	printf("数据库创建成功\n");

	return 0;
	//}}}
}


//服务器ip和端口创建
int ser(int sfd)
{
	//{{{
	//socket
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

	//bind
	//填充信息
	struct sockaddr_in sin;

	sin.sin_family=AF_INET;
	sin.sin_port=htons(PORT);
	sin.sin_addr.s_addr=inet_addr(IP);

	if(bind(sfd,(void*)&sin,sizeof(sin))<0)
	{
		ERR_LOG("bind");
		return -1;
	}

	//监听
	if(listen(sfd,5)<0)
	{
		ERR_LOG("listen");
		return -1;
	}

	return sfd;
	//}}}
}

//线程
void *callBackHandler(void *arg)
{
	sqlite3 *db=NULL;
	sqlite3_open("./personnel.db",&db);
	__msgInfo mmm=(*(__msgInfo *)arg);
	int newfd=mmm.newfd;
	char buf[N]="";
	char name[N]="";
	char his[N]="";
	int st=0;
	while(1)
	{
		bzero(buf,sizeof(buf));
		int recv_len=recv(newfd,buf,N,0);
		if(recv_len<0)
		{
			ERR_LOG("recv");
			break;
		}
		else if(0==recv_len)
		{
			printf("对方关闭\n");
			break;
		}

		switch(buf[0])
		{
		case 'R':
			do_REG(newfd,db);//注册
			break;
		case 'L':
			/*	if(5==do_LOGIN(newfd,db,name,&st))//登录
				{
				do_up_login(newfd,db,name,&st,his);//登陆后的功能
				}*/
			break;
		case 'E':
			goto END; //退出

			break;
		default:
			printf("输入错误\n");

		}

	}
END:
	close(newfd);
	pthread_exit(NULL);//退出
	return 0;

}

//注册
int do_REG(int newfd,sqlite3 *db)
{
	//{{{
	char buf[N]="";
	char name[N]="";
	char his[N]="";
	int st=0;
	while(1)
	{
		bzero(buf,sizeof(buf));
		int recv_len=recv(newfd,buf,N,0);
		if(recv_len<0)
		{
			ERR_LOG("recv");
			break;
		}
		else if(0==recv_len)
		{
			printf("对方关闭\n");
			break;
		}

		switch(buf[0])
		{
		case 1:
			do_user(newfd,db);//注册普通用户
			break;
		case 2:
			do_root(newfd,db);//注册管理用户
			break;
		case 0:     
			return -1;	//返回上级
		default:
			printf("输入错误\n");

		}

	}
	return 0;
//}}}
}

//注册普通用户
int do_user(int newfd,sqlite3 *db)
{

	//{{{
	char sql[512]="";
	MSG msg;
	int exe;
	char *errmsg;
	memset(&msg,0,sizeof(msg));
	int recv_reg=recv(newfd,&msg,sizeof(msg),0);  //读取客户端的用户信息
	sprintf(sql,"insert into user values('%s','%s','%s',%d,'%s','%s',%d,%d,%d,%d)",msg.use,msg.password,msg.username,msg.age,msg.sex,msg.address,msg.pthone,msg.level,msg.money,msg.state);
	exe=sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if(exe!=0)
	{
		printf("账号 %s 注册失败 (原因已反馈)\n",msg.use);
		printf("%s\n",errmsg);
		char buf2[N]="注册无效(重复注册)";
		send(newfd,buf2,sizeof(buf2),0);
		return -1;
	}
	else
	{
		//反馈客户端
		char buf[N]="注册成功";
		send(newfd,buf,sizeof(buf),0);

		printf("用户 %s  账号 %s 的信息已插入信息表\n",msg.username,msg.use);
	}


	return 0;
	//}}}
}


int do_root(int newfd,sqlite3 *db)
{

	//{{{
	char sql[512]="";
	ROOT msg;
	int exe;
	char *errmsg;
	memset(&msg,0,sizeof(msg));
	int recv_reg=recv(newfd,&msg,sizeof(msg),0);  //读取客户端的用户信息

	if(msg.key != 666)
	{
		printf("账号 %s 注册失败 (原因已反馈)\n",msg.use);
		char buf3[N]=" 注册码无效  请联系管理员获取  或 阅读本软件README ";
		send(newfd,buf3,sizeof(buf3),0);
		return -1;

	}
	sprintf(sql,"insert into root values('%s','%s','%s',%d,%d)",msg.use,msg.password,msg.username,msg.pthone,msg.state);
	exe=sqlite3_exec(db,sql,NULL,NULL,&errmsg);
	if(exe!=0)
	{
		printf("账号 %s 注册失败 (原因已反馈)\n",msg.use);
		printf("%s\n",errmsg);
		char buf2[N]="注册无效(账号重复注册)";
		send(newfd,buf2,sizeof(buf2),0);
		return -1;
	}
	else
	{
		//反馈客户端
		char buf[N]="注册成功";
		send(newfd,buf,sizeof(buf),0);

		printf("用户 %s,账号 %s  的信息已插入信息表\n",msg.username,msg.use);
	}

	return 0;
	//}}}
}


