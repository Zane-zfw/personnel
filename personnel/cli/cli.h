/*
 * @Author: your name
 * @Date: 2021-08-11 13:06:06
 * @LastEditTime: 2021-08-11 13:10:58
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \user1\cli.h
 */
#ifndef __CLI_H__
#define __CLI_H__

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
int do_dele_data(int sfd);
int do_up_data(int sfd);
#define N 128
#define IP "0.0.0.0"
#define PORT 8888

#define REG 'R'//注册指令
#define LOGIN 'L' //登录指令
#define QUERY 'Q' //查询单词
#define HISTORY 'H' //查询历史
#define EXIT 'E' //退出
#define DELE 'D' //删除
#define UP 'U' //修改
#endif
