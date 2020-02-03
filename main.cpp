#include <iostream>
#include <string>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <vector>
#include <regex>
#include <fstream>
//#pragma comment (lib,"ws2_32")
#define MAXBUFSIZE 8192
using namespace std;
#include "chconf.h"
typedef struct childDataType{
    int index;
	SOCKET* client=NULL;
    bool ready=false;//就绪状态
}ChildType;
vector<ChildType *> taskThreadList;//任务进程链表
typedef struct proxyRuleType{
    int port;
    string host;
	vector<regex> reg;
}ProxyRule;
vector<ProxyRule> proxyRule;//转发规则
typedef struct proxyPort_Config{
    vector<string> sendStr;
    int port=0;
    int debug=0;
    int initThread=0;
    int timeout;
}proxyPortConfig;
proxyPortConfig config;

void delay(int ms){HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);WaitForSingleObject(hEvent,ms);}//延时函数

string HexToAscii(unsigned char pHex[], int nLen){
    string str;
    unsigned char Nibble[2];
    int i,j;
    for (i = 0; i < nLen; i++){
        Nibble[0] = (pHex[i] & 0xF0) >> 4;
        Nibble[1] = pHex[i] & 0x0F;
        str+="\\x";
        for (j = 0; j < 2; j++){
            if (Nibble[j] < 10)
                Nibble[j] += 0x30;
            else
                if (Nibble[j] < 16)
                    Nibble[j] = Nibble[j] - 10 + 'A';
            str+=Nibble[j];
        }
    }
    return str;
}
DWORD WINAPI Child(LPVOID lpParamter){
	ChildType* task = (ChildType *)lpParamter;
	if(config.debug>2)printf("线程%d已启动！\n",task->index);
	while(1){
		if(task->client!=NULL){
            task->ready=false;
            if(config.debug>1)printf("线程%d收到！\n",task->index);
            SOCKET ClientSock=*task->client;
            int timeout = config.timeout;
            setsockopt(ClientSock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
            char RecvBuf[MAXBUFSIZE];
            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
            int nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
            if(config.debug>10)printf("recv返回长度：%d，内容：%s|||\n",nRecv,HexToAscii((unsigned char*)RecvBuf,nRecv).c_str());

            int sendStrSize=config.sendStr.size();
            if(nRecv <= 0)
                for(int i=0;i<sendStrSize;i++){
                    if (nRecv <= 0){
                        int err = WSAGetLastError();
                        if (err == EWOULDBLOCK || err == WSAETIMEDOUT){
                            send(ClientSock, config.sendStr[i].c_str(), config.sendStr[i].size(), 0);
                            if(config.debug>2)printf("类型尝试发送：%s\n",config.sendStr[i].c_str());
                            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
                            nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);//主动发送继续接收
                        }else{
                            if(config.debug>2)printf("客户端断开！\n");
                            closesocket(ClientSock);
                            task->client=NULL;
                            task->ready=true;
                            continue;
                        }
                    }else break;
                }
            if (nRecv <= 0){
                if(config.debug>2)printf("客户端类型未识别！已断开！\n");
                closesocket(ClientSock);
                task->client=NULL;
                task->ready=true;
                continue;
            }

            if(config.debug>2)printf("收到：%s\n",RecvBuf);
            int nn=proxyRule.size();
            int connect_port=0;
            string connect_host;
            for(int i=0;i<nn;i++){
                int nn2=proxyRule[i].reg.size();
                for(int j=0;j<nn2;j++){
                    if(regex_match(RecvBuf,proxyRule[i].reg[j])){
                        connect_port=proxyRule[i].port;
                        connect_host=proxyRule[i].host;
                        break;
                    }
                }
                if(connect_port!=0)break;
            }
            if(connect_port==0){
                if(config.debug>2)printf("未知类型，收到：%s|\n",RecvBuf);
                closesocket(ClientSock);
                task->client=NULL;
                task->ready=true;
                continue;
            }
            if(config.debug>2)printf("转发成功：%s:%d\n",connect_host.c_str(),connect_port);
			SOCKET s = socket(AF_INET,SOCK_STREAM,0);
			if(s == INVALID_SOCKET){
				if(config.debug>2)printf("socket链接错误！%d\n",WSAGetLastError());
				break;//客户端断开
			}
			struct hostent *pt = gethostbyname(connect_host.c_str());//解析域名IP
			if(!pt)break;//客户端断开
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
			sockaddr.sin_port = htons(connect_port);//设置要连接的IP和端口
			int ret;
			ret=connect(s, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if(ret == SOCKET_ERROR)break;//客户端断开
			if(s <= 0){
				if(config.debug>2)printf("socket connect失败！%d\n",WSAGetLastError());
				break;//客户端断开
			}
			SOCKET ServerSock=s;
            send(ServerSock,RecvBuf,nRecv,0);
            //printf("转发第一次数据：%s\n",RecvBuf);
            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
			fd_set Fd_Read;
			while(1){
				FD_ZERO(&Fd_Read);
				FD_SET(ClientSock,&Fd_Read);
				FD_SET(ServerSock,&Fd_Read);
				ret = select(0,&Fd_Read,NULL,NULL,NULL);
				if(ret <= 0){printf("Error: select失败！\n");break;}
				if(FD_ISSET(ClientSock, & Fd_Read)) {
					nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
					if(nRecv <= 0){
						//printf("客户端断开！\n");
						break;
					}
					if(config.debug>3)printf("ClientSock:%s\n",RecvBuf);
					send(ServerSock, RecvBuf, nRecv, 0);
					memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
				}
				if(FD_ISSET(ServerSock, & Fd_Read)) {
					nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("客户端断开！\n");
						break;
					}
					if(config.debug>3)printf("ServerSock:%s\n",RecvBuf);
					send(ClientSock, RecvBuf, nRecv, 0);
					memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
				}
			}
			closesocket(ServerSock);
			closesocket(ClientSock);
			task->client=NULL;
			task->ready=true;
			if(config.debug>1)printf("客户端断开，线程已就绪\n");
		}
		delay(5);//降低cpu
	}
    return 0L;
}
DWORD WINAPI Main(LPVOID lpParamter){
    return 0L;
}
int createNewTaskThread(ChildType* t){
	t->index=taskThreadList.size();
	taskThreadList.push_back(t);
	HANDLE hThread = CreateThread(NULL, 0, Child, t, 0, NULL);//启动子线程
    CloseHandle(hThread);
	return 1;
}
void sendTask(SOCKET* client){
	//派发数据
	//选线程池
	int k=taskThreadList.size();
	for(int i=0;i<k;i++){
		if(taskThreadList[i]->ready){
			taskThreadList[i]->ready=false;
			taskThreadList[i]->client=client;
			return;
		}
    }
	ChildType* t=new ChildType;
	t->client=client;
	createNewTaskThread(t);
}
bool is_numeric(string str){
    int k=str.size();
    for(int i=0;i<k;i++){
        int tmp = (int)str[i];
        if (tmp >= 48 && tmp <= 57)continue;else return false;
    }
    return true;
}
string GetExePath(){
    char szFilePath[MAX_PATH + 1]={0};
    GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
    (strrchr(szFilePath, '\\'))[0] = 0; // 删除文件名，只获得路径字串
    string path = szFilePath;
    return path;
}
bool init_config(){

    //ch_conf conf("D:\\文档\\codeblock\\ProxyPort2\\bin\\Debug\\portProxy.conf");
    //ch_conf conf2("D:\\文档\\codeblock\\ProxyPort2\\bin\\Debug\\portType.conf");
    string path=GetExePath();
    ch_conf conf(path+"\\portProxy.conf");
    ch_conf conf2(path+"\\portType.conf");
    if(!conf.verifyKeyValue("general")){
        printf("Error: 配置文件加载失败！Key-Value格式不正确！\n");
        return false;
    }
    string confstr;
    /////////////////////////////////////////////////////////////
    //设置端口
    confstr=conf.getKeyValue("general","port");
    if(!is_numeric(confstr)){
        printf("Error: %s监听端口号必须是数字！",confstr.c_str());
        return false;
    }
    if(atoi(confstr.c_str())<1||atoi(confstr.c_str())>65535){
        printf("Error: %s监听端口号只能在1到65535之间！",confstr.c_str());
        return false;
    }
    config.port=atoi(confstr.c_str());//设置端口号
    /////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////
    //设置调试
    confstr=conf.getKeyValue("general","debug");
    if(!is_numeric(confstr)){
        printf("Error: %s调试开关必须是数字！",confstr.c_str());
        return false;
    }
    config.debug=atoi(confstr.c_str());
    if(config.debug>0)printf("Warn: 调试模式已经打开！%d\n",config.debug);

    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //设置最小线程
    confstr=conf.getKeyValue("general","initThread");
    if(!is_numeric(confstr)){
        printf("Error: %s初使线程必须是数字！",confstr.c_str());
        return false;
    }
    config.initThread=atoi(confstr.c_str());
    for(int i=0;i<config.initThread;i++){
        ChildType* t=new ChildType;
        t->ready=true;
        createNewTaskThread(t);
        delay(5);
    }
    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //设置被动模式超时时间
    confstr=conf.getKeyValue("general","timeout");
    if(!is_numeric(confstr)){
        printf("Error: %s超时时间必须是数字！",confstr.c_str());
        return false;
    }
    config.timeout=atoi(confstr.c_str());
    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //设置测试文本
    vector<string> str_new;
    config.sendStr=str_new;
    vector<string> str=conf.readValue("test");
    int k=str.size();
    for(int i=0;i<k;i++){
        string s;
        s=str[i];
        conf.replace(s,"\\\\","\\");
        conf.replace(s,"\\r","\r");
        conf.replace(s,"\\t","\t");
        conf.replace(s,"\\n","\n");
        config.sendStr.push_back(s);
    }
    /////////////////////////////////////////////////////////////

    //设置端口转发规则


    vector<ProxyRule> proxyRule_new;
    proxyRule=proxyRule_new;
    int j=conf.size();
    for(int i=0;i<j;i++){
        ProxyRule rule;
        CHConfigType t=conf[i];
        string port=t.name;
        if(port=="test")continue;
        if(port=="general")continue;
        string host;

        if(!is_numeric(port)){
            unsigned int k=port.find(":");
            if(k==port.npos){
                host=port;
                port="80";
            }else{
                host=port.substr(0,k);
                port=port.substr(k+1);
            }
        }else host="127.0.0.1";
        rule.host=host;
        rule.port=atoi(port.c_str());
        if(rule.port<1||rule.port>65535){
            printf("Warn: %d端口号只能在1到65535之间！",rule.port);
            continue;
        }
        int k=t.value.size();
        //自定义的优先
        for(int n=0;n<k;n++)rule.reg.push_back(regex(t.value[n]));
        vector<string> extra=conf.getExtraArrSplitedBySpace(t.name);
        k=extra.size();
        //printf("Debug: ",);
        for(int n=0;n<k;n++){
            vector<string> r=conf2[extra[n]];
            int m=r.size();
            for(int l=0;l<m;l++)rule.reg.push_back(regex(r[l]));
        }
        proxyRule.push_back(rule);
    }
    /////////////////////////////////////////////////////////////
    return true;
}
void init_socket(){
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		printf("Error: socket初始化失败！\n");
		return;
	};//初始化
	SOCKET  s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET){// == SOCKET_ERROR
		printf("Error: socket执行失败！\n");
		return;
	}
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("0.0.0.0");
    sockaddr.sin_port = htons(config.port);
	int n;
	n=bind(s,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in));
	if (n==SOCKET_ERROR) {// == SOCKET_ERROR
		printf("Error: 端口%d绑定失败！\n",config.port);
		return;
	}else printf("端口绑定成功：%d\n",config.port);

    n=listen(s,3);//开始监听//1
	if (n<0){// == SOCKET_ERROR
		printf("Error: 监听失败！\n");
		return;
	}
	printf("启动成功！\n");
    while(1){
		SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//等待客户端连接 阻塞
		SOCKET *client = new SOCKET;
		*client=client_tmp;
		sendTask(client);//分配线程任务
		if(config.debug>1)printf("分配线程！\n");
	}
    closesocket(s);
    WSACleanup();
}
int main(int argc,char *argv[]){
	printf("ProxyPort: 启动中...\n");
    if(!init_config())return 1;//初始化配置文件
    init_socket();
    return 0;
}
