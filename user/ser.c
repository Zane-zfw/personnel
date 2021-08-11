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
#define DELE 'D'//删除
#define UP 'U'	//修改


int up_sqlite3(sqlite3 *);
int ser(int sfd);
int do_REG(int newfd,sqlite3 *);
int do_user(int newfd,sqlite3 *);
int do_root(int newfd,sqlite3 *);
void *callBackHandler(void *arg);
int do_LOGIN(int newfd,sqlite3 *,char *name,int *st);
int do_userlogin(int newfd,sqlite3 *,char *name,int *st);
int do_rootlogin(int newfd,sqlite3 *,char *name,int *st);
int do_up_login(int newfd,sqlite3 *,char *name,int *st);
int do_query(int newfd,sqlite3 *,char *name);
int do_up_root(int newfd,sqlite3 *,char *name,int *st);
int do_query_all(int newfd,sqlite3 *,char *name);
int do_dele_data(int newfd,sqlite3 *,char *name);
int do_up_data(int newfd,sqlite3 *,char *name);

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
	char level[15];//保存员工等级
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
	//{{{
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
			do_LOGIN(newfd,db,name,&st);	//登录
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
	//}}}
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
//}}}z
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
	sprintf(sql,"insert into user values('%s','%s','%s',%d,'%s','%s',%d,'%s',%d,%d)",msg.use,msg.password,msg.username,msg.age,msg.sex,msg.address,msg.pthone,msg.level,msg.money,msg.state);
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

//注册管理用户
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

//登录
int do_LOGIN(int newfd,sqlite3 *db,char *name,int *st)
{
	//{{{
	
	char buf[N]="";
	char his[N]="";
	int j;
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
			j=do_userlogin(newfd,db,name,st);//登录普通用户
			if(5==j)
			{
				do_up_login(newfd,db,name,st);//登陆后的功能
			}

			else if(6==j)
			{
				return -1;
			}
			break;
		case 2:
			if(5==do_rootlogin(newfd,db,name,st))//管理员登录
			{
				do_up_root(newfd,db,name,st);
			}
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


//登录普通用户
int do_userlogin(int newfd,sqlite3 *db,char *name,int *st)
{
	//{{{
	char *errmsg=NULL;
	char buf[N]="";
	int i,j;
	char sql[N]="";
	char sqll[N]="";
	char ssq[N]="";
	char buff[N]="该用户未注册，请注册后再来尝试登录";
	char aaa[N]="密码不正确";
	char **dpresult;
	int row,column;
	int ii=1;

	MSG msg;

	int recv_reg=recv(newfd,&msg,sizeof(msg),0);  //读取客户端的用户信息
	strcpy(name,msg.use);
	memcpy(&st,&(msg.state),4);
	sprintf(sql,"select *from user where use='%s'",msg.use);
	sqlite3_get_table(db,sql,&dpresult,&row,&column,&errmsg);

	if(row==0)
	{
		printf("用户 %s 未注册(错误已反馈)\n",msg.username);
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{
		//跟密码做比较
		int str;
		str=strcmp(msg.password,dpresult[11]);
		if(str!=0)
		{
			//密码错误
			printf("账号 %s 的密码错误(错误已反馈)\n",msg.use);

			ii++;
			if(ii==3)
			{
				return 6;
			}

			send(newfd,aaa,sizeof(aaa),0);
		}
		else
		{	
			int ato;
			ato=atoi(dpresult[19]);
			//比较状态位
			if(ato==1)
			{   
				//重复登录
				printf("账号 %s 重复登录(错误已反馈)\n",msg.use);
				char q[N]="重复登录，请重新登录";
				send(newfd,q,sizeof(q),0);
			}

			else
			{
				//登录成功 
				msg.state=1;
				sprintf(ssq,"update user set state=%d where use='%s'",msg.state,msg.use);
				if(sqlite3_exec(db,ssq,NULL,NULL,&errmsg)==0);
				{
					printf("登录状态修改成功\n");
				}
				char qq[N]="登录成功";
				send(newfd,qq,sizeof(qq),0);
				sqlite3_free_table(dpresult);//释放
				return 5;
			}

		}
	}	
	
	
	
	return 0;
	//}}}
}

//管理员登录
int do_rootlogin(int newfd,sqlite3 *db,char *name,int *st)
{
	//{{{
	
	char *errmsg=NULL;
	char buf[N]="";
	int i,j;
	char sql[N]="";
	char sqll[N]="";
	char ssq[N]="";
	char buff[N]="该用户未注册，请注册后再来尝试登录";
	char aaa[N]="密码不正确";
	char **dpresult;
	int row,column;

	ROOT msg;

	int recv_reg=recv(newfd,&msg,sizeof(msg),0);  //读取客户端的用户信息
	strcpy(name,msg.use);
	memcpy(&st,&(msg.state),4);
	sprintf(sql,"select *from root where use='%s'",msg.use);
	sqlite3_get_table(db,sql,&dpresult,&row,&column,&errmsg);

	if(row==0)
	{
		printf("用户 %s 未注册(错误已反馈)\n",msg.username);
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{
		//跟密码做比较
		int str;
		str=strcmp(msg.password,dpresult[6]);
		if(str!=0)
		{
			//密码错误
			printf("账号 %s 的密码错误(错误已反馈)\n",msg.use);
			send(newfd,aaa,sizeof(aaa),0);
		}
		else
		{	
			int ato;
			ato=atoi(dpresult[9]);
			//比较状态位
			if(ato==1)
			{   
				//重复登录
				printf("账号 %s 重复登录(错误已反馈)\n",msg.use);
				char q[N]="重复登录，请重新登录";
				send(newfd,q,sizeof(q),0);
			}

			else
			{
				//登录成功 
				msg.state=1;
				sprintf(ssq,"update root set state=%d where use='%s'",msg.state,msg.use);
				if(sqlite3_exec(db,ssq,NULL,NULL,&errmsg)==0);
				{
					printf("登录状态修改成功\n");
				}
				char qq[N]="登录成功";
				send(newfd,qq,sizeof(qq),0);
				sqlite3_free_table(dpresult);//释放
				return 5;
			}

		}
	}	
	
	
	return 0;
	//}}}
}

//普通管理
int do_up_login(int newfd,sqlite3 *db,char *name,int *st)
{
	//{{{
	char buf[N]="";
	char ssq[N]="";
	char *errmsg=NULL;

	while(1)
	{
		bzero(buf,sizeof(buf));
		int recv_len=recv(newfd,buf,N,0);//读取客户端指令
		if(recv_len<0)
		{
			ERR_LOG("recv");
			break;
		}
		else if(0==recv_len)
		{
			printf("退出登录\n");
			break;
		}
		//------------------------------
		switch(buf[0])
		{
		case 'Q':
			//查信息
			do_query(newfd,db,name);
			break;
		case 'H':
			//查记录
		//	do_up_history(newfd,dp,db,name,his);
			break;
		case 'E':
			//退出
			*st=0;
			sprintf(ssq,"update user set state=%d where use='%s'",*st,name);
			if(sqlite3_exec(db,ssq,NULL,NULL,&errmsg)==0);
			{
				printf("退出状态修改成功\n");
			}
			return -1;
		}
	}	
	
	
	return 0;
	//}}}
}


//查信息
int do_query(int newfd,sqlite3 *db,char *name)
{
	//{{{
	char english[N]="";
	bzero(english,sizeof(english));
	int recv_len=recv(newfd,english,N,0);//读取客户端英文单词

	char *errmsg=NULL;
	char sql[512]="";
	char buff[N]="抱歉，该用户没有信息";
	char **dpresult;
	int row,column;
	sprintf(sql,"select *from user where use='%s'",english);
	if(sqlite3_get_table(db,sql,&dpresult,&row,&column,&errmsg))//查找单词
	{
		printf("查询报错\n");
	}
	if(row==0)
	{
		printf("用户 %s 未找到(错误已反馈)\n",english);
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{
		char buf[N]="";
		char sqll[512]="";
		time_t curtime;
		char sqq[N]="查询完毕";
		int i=1,j=0;
		strcat(buf,"姓名:");
		strcat(buf,dpresult[12]);
		strcat(buf,"   ");
		strcat(buf,"年龄:");
		strcat(buf,dpresult[13]);
		strcat(buf,"   ");
		strcat(buf,"性别:");
		strcat(buf,dpresult[14]);
		strcat(buf,"\n");
		strcat(buf,"地址:");
		strcat(buf,dpresult[15]);
		strcat(buf,"   ");
		strcat(buf,"电话:");
		strcat(buf,dpresult[16]);
		strcat(buf,"   ");
		strcat(buf,"等级:");
		strcat(buf,dpresult[17]);
		strcat(buf,"   ");
		strcat(buf,"工资:");
		strcat(buf,dpresult[18]);
		strcat(buf,"\n");
		strcat(buf,"\n");
		//查询完毕
		send(newfd,buf,sizeof(buf),0);

		}
	return 0;
	//}}}
}

//管理员模式
int do_up_root(int newfd,sqlite3 *db,char *name,int *st)
{
	//{{{
	
	char buf[N]="";
	char ssq[N]="";
	char *errmsg=NULL;

	while(1)
	{
		bzero(buf,sizeof(buf));
		int recv_len=recv(newfd,buf,N,0);//读取客户端指令
		if(recv_len<0)
		{
			ERR_LOG("recv");
			break;
		}
		else if(0==recv_len)
		{
			printf("退出登录\n");
			break;
		}
		//------------------------------
		switch(buf[0])
		{
		case 'Q':
			//查信息
			do_query_all(newfd,db,name);
			break;
		case 'U':
			//修改信息
			do_up_data(newfd,db,name);
			break;
		case 'D':
			//删除信息
			do_dele_data(newfd,db,name);
			break;
		case 'E':
			//退出
			*st=0;
			sprintf(ssq,"update root set state=%d where use='%s'",*st,name);
			if(sqlite3_exec(db,ssq,NULL,NULL,&errmsg)==0);
			{
				printf("退出状态修改成功\n");
			}
			return -1;
		}
	}	
	
	
	return 0;
	//}}}
}


//查询所有信息
int do_query_all(int newfd,sqlite3 *db,char *name)
{
	//{{{
	char *errmsg=NULL;
	char sql[256]="";
	char ssq[128]="";
	char buff[N]="抱歉，该用户没有信息";
	char **dpresult;
	char **dpresult1;
	int row,column;
	int row1,column1;

	sprintf(sql,"select *from user");
	if(sqlite3_get_table(db,sql,&dpresult,&row,&column,&errmsg))//查找用户表
	{
		printf("查询报错\n");
	}

	sprintf(ssq,"select *from root");
	sqlite3_get_table(db,ssq,&dpresult1,&row1,&column1,&errmsg);//查找管理表

	if(row==0)
	{
		printf("用户  未找到(错误已反馈)\n");
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{
	
		char buf[256]="";
		char sqll[256]="";
		char bbuf[256]="";
		time_t curtime;
		char sqq[N]="查询完毕";
		int i=1,j=0;
		int a=1,c=0;

		bzero(buf,sizeof(buf));
		for(i;i<row+1;i++)
		{


			strcat(buf,"普通用户>> ");
			strcat(buf,"账号:");
			strcat(buf,dpresult[i*column+j]);
			strcat(buf,"   ");
			strcat(buf,"密码:");
			strcat(buf,dpresult[i*column+j+1]);
			strcat(buf,"   ");
			strcat(buf,"姓名:");
			strcat(buf,dpresult[i*column+j+2]);
			strcat(buf,"   ");
			strcat(buf,"年龄:");
			strcat(buf,dpresult[i*column+j+3]);
			strcat(buf,"   ");
			strcat(buf,"性别:");
			strcat(buf,dpresult[i*column+j+4]);
			strcat(buf,"\n");
			strcat(buf,"	地址:");
			strcat(buf,dpresult[i*column+j+5]);
			strcat(buf,"   ");
			strcat(buf,"电话:");
			strcat(buf,dpresult[i*column+j+6]);
			strcat(buf,"   ");
			strcat(buf,"等级:");
			strcat(buf,dpresult[i*column+j+7]);
			strcat(buf,"   ");
			strcat(buf,"工资:");
			strcat(buf,dpresult[i*column+j+8]);
			strcat(buf,"\n");
			//发送信息
			send(newfd,buf,sizeof(buf),0);
			bzero(buf,sizeof(buf));
		}
	
		for(a;a<row1+1;a++)
		{
			bzero(bbuf,sizeof(bbuf));
			strcat(bbuf,"管理用户>> ");
			strcat(bbuf,"账号:");
			strcat(bbuf,dpresult1[a*column1+c]);
			strcat(bbuf,"   ");
			strcat(bbuf,"密码:");
			strcat(bbuf,dpresult1[a*column1+c+1]);
			strcat(bbuf,"   ");
			strcat(bbuf,"姓名:");
			strcat(bbuf,dpresult1[a*column1+c+2]);
			strcat(bbuf,"   ");
			strcat(bbuf,"电话:");
			strcat(bbuf,dpresult1[a*column1+c+3]);
			strcat(bbuf,"   ");
			strcat(bbuf,"\n");
			//发送信息
			send(newfd,bbuf,sizeof(bbuf),0);
			bzero(bbuf,sizeof(bbuf));
		}
		//完毕
		send(newfd,sqq,sizeof(sqq),0);
		bzero(sqq,sizeof(sqq));
	}
	
	return 0;
	//}}}
}


//删除信息
int do_dele_data(int newfd,sqlite3 *db,char *name)
{
	//{{{
	
	char english[N]="";
	bzero(english,sizeof(english));
	int recv_len=recv(newfd,english,N,0);//读取客户段

	char *errmsg=NULL;
	char sql[512]="";
	char sqll[512]="";
	char buff[N]="没有该用户  不必删除";
	char **dpresult;
	int row,column;

	sprintf(sqll,"select *from user where use='%s'",english);
	if(sqlite3_get_table(db,sqll,&dpresult,&row,&column,&errmsg))//查找单词
	{
		printf("查询报错\n");
	}
	if(row==0)
	{
		printf("没有该用户 不必删除\n");
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{

	sprintf(sql,"delete from user where use = '%s'",english);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg))//删除信息
	{
		printf("删除报错\n");
	}
		char sqq[N]="删除成功";

		//删除完毕
		send(newfd,sqq,sizeof(sqq),0);
		printf("删除成功\n");
	}
	
	return -1;
	//}}}
}


//修改信息
int do_up_data(int newfd,sqlite3 *db,char *name)
{
	//{{{
	
	char english[N]="";
	bzero(english,sizeof(english));
	int recv_len=recv(newfd,english,N,0);//读取客户段

	char *errmsg=NULL;
	char sql[512]="";
	char sqll[512]="";
	char buff[N]="没有该用户，不必修改";
	char **dpresult;
	int row,column;
	
	sprintf(sqll,"select *from user where use='%s'",english);
	if(sqlite3_get_table(db,sqll,&dpresult,&row,&column,&errmsg))//查找单词
	{
		printf("查询报错\n");
	}
	
	if(row==0)
	{
		printf("没有该用户 不必修改\n");
		send(newfd,buff,sizeof(buff),0);
	}
	else
	{
		
		char buf[N]="";
		char sqq[N]="查询完毕";
		int i=1,j=0;
		bzero(buf,sizeof(buf));
		strcat(buf,"姓名:");
		strcat(buf,dpresult[12]);
		strcat(buf,"   ");
		strcat(buf,"年龄:");
		strcat(buf,dpresult[13]);
		strcat(buf,"   ");
		strcat(buf,"性别:");
		strcat(buf,dpresult[14]);
		strcat(buf,"\n");
		strcat(buf,"地址:");
		strcat(buf,dpresult[15]);
		strcat(buf,"   ");
		strcat(buf,"电话:");
		strcat(buf,dpresult[16]);
		strcat(buf,"   ");
		strcat(buf,"等级:");
		strcat(buf,dpresult[17]);
		strcat(buf,"   ");
		strcat(buf,"工资:");
		strcat(buf,dpresult[18]);
		strcat(buf,"\n");
		strcat(buf,"\n");
		//查询完毕
		send(newfd,buf,sizeof(buf),0);
	}


	char sqql[128]="";
	char sqql1[128]="";
	char sqql2[128]="";
	char sqql3[128]="";
	char sqql4[128]="";
	char sqql5[128]="";
	char sqql6[128]="";
	MSG msg;
	int exe;
	memset(&msg,0,sizeof(msg));
	int recv_reg=recv(newfd,&msg,sizeof(msg),0);  //读取客户端的用户信息
	//sprintf(sqql,"update user set name='%s' and age='%d' and sex='%s' and address='%s' and pthone='%d' and level='%s' and money='%d' where use='%s'",\
	//																		msg.username,msg.age,msg.sex,msg.address,msg.pthone,msg.level,msg.money,msg.use);
	sprintf(sqql,"update user set name='%s' where use='%s'",msg.username,msg.use);
	sprintf(sqql1,"update user set age='%d' where use='%s'",msg.age,msg.use);
	sprintf(sqql2,"update user set sex='%s' where use='%s'",msg.sex,msg.use);
	sprintf(sqql3,"update user set address='%s' where use='%s'",msg.address,msg.use);
	sprintf(sqql4,"update user set pthone='%d' where use='%s'",msg.pthone,msg.use);
	sprintf(sqql5,"update user set level='%s' where use='%s'",msg.level,msg.use);
	sprintf(sqql6,"update user set money='%d' where use='%s'",msg.money,msg.use);
	sqlite3_exec(db,sqql1,NULL,NULL,&errmsg);
	sqlite3_exec(db,sqql2,NULL,NULL,&errmsg);
	sqlite3_exec(db,sqql3,NULL,NULL,&errmsg);
	sqlite3_exec(db,sqql4,NULL,NULL,&errmsg);
	sqlite3_exec(db,sqql5,NULL,NULL,&errmsg);
	sqlite3_exec(db,sqql6,NULL,NULL,&errmsg);

	exe=sqlite3_exec(db,sqql,NULL,NULL,&errmsg);
	if(exe!=0)
	{
		printf("账号 %s 修改失败 (原因已反馈)\n",msg.use);
		printf("%s\n",errmsg);
		char buf2[N]="修改失败";
		send(newfd,buf2,sizeof(buf2),0);
		return -1;
	}
	else
	{
		//反馈客户端
		char buf[N]="修改成功";
		send(newfd,buf,sizeof(buf),0);
		printf("用户 %s  账号 %s 的信息已修改\n",msg.username,msg.use);
	}

	return 0;
	//}}}
}
