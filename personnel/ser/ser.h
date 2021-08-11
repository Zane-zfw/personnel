/*
 * @Author: your name
 * @Date: 2021-08-11 13:05:24
 * @LastEditTime: 2021-08-11 13:11:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \user1\ser.h
 */
#ifndef __SER_H__
#define __SER_H__

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


#endif
