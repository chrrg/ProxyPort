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
#include "md5.h"
typedef struct childDataType{
    HANDLE handle;
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
vector<string> allowIP;//转发规则
typedef struct proxyPort_Config{
    vector<string> sendStr;
    int port=0;
    int debug=0;
    int initThread=0;
    int timeout;
    string forwardHost="";
    int forwardPort=0;
    bool is_Auth;//是否需要授权
    string Auth_pwd;//授权密码
    string unAuthHost;//未授权转发主机
    int unAuthPort;//未授权转发端口
}proxyPortConfig;
proxyPortConfig config;
fstream access_log;
void delay(int ms){HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);WaitForSingleObject(hEvent,ms);}//延时函数
string getTime(){
	time_t t;
    time(&t);
    char p[64];
    strftime(p, sizeof(p), "%Y-%m-%d %H:%M:%S", localtime(&t));
	return p;
}
void wlog(string str){
    access_log<<getTime()<<"|"<<str<<endl;
}
bool isLogin(string ip){
    int allowIPSize=allowIP.size();
    for(int i=0;i<allowIPSize;i++)
        if(allowIP[i]==ip)//如果允许列表中有当前连接的ip
            return true;
    return false;
}
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
std::vector<std::string> split(const std::string& s, char delimiter){
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while(std::getline(tokenStream, token, delimiter))tokens.push_back(token);
   return tokens;
}
DWORD WINAPI Child(LPVOID lpParamter){
	ChildType* task = (ChildType *)lpParamter;
	if(config.debug>2)printf("线程%d已启动！\n",task->index);
	//wlog("线程"+to_string(task->index)+"已启动！");
	while(1){
		if(task->client!=NULL){
		    struct sockaddr_in client_sockaddr;
		    string log_tmp;
            int len=sizeof(client_sockaddr);
            string client_addr;
            if(!getpeername(*task->client, (struct sockaddr *)&client_sockaddr, &len)){
                client_addr=inet_ntoa(client_sockaddr.sin_addr);
                client_addr+=":";
                client_addr+=to_string(ntohs(client_sockaddr.sin_port));
            }
            task->ready=false;
            if(config.debug>1)printf("线程%d收到！\n",task->index);
            SOCKET ClientSock=*task->client;
            int timeout = config.timeout;
            setsockopt(ClientSock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
            char RecvBuf[MAXBUFSIZE];
            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
            int connect_port=0;
            string connect_host;
            bool isNeedRecv=false;
            bool isNeedSend=false;
            int NeedSendLength=0;
            char NeedSendStr[MAXBUFSIZE];

            int nRecv=0;
            SOCKET ServerSock;
            struct hostent*pt=NULL;
            if(config.forwardPort==0){//无条件转发端口如果是0 说明需要验证
                //说明需要授权才能访问
                nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
                if(config.debug>10)printf("recv返回长度：%d，内容：%s|||\n",nRecv,HexToAscii((unsigned char*)RecvBuf,nRecv).c_str());
                //是否需要授权 且获取到数据且未验证
                bool verify=isLogin(inet_ntoa(client_sockaddr.sin_addr));
                string loginStr=RecvBuf;
                if(config.is_Auth&&nRecv > 0&&!verify&&loginStr=="ProxyPort2-Login"){//需要授权，且未登录则需要判断是否是登录
                    MD5 md5;
                    string ipHash;
                    md5.update("ProxyPort2_IP_"+client_addr);
                    ipHash=md5.toString();
                    //printf("loginStr:%s\n",loginStr.c_str());
                    send(ClientSock, ipHash.c_str(), ipHash.size(), 0);
                    nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
                    if(nRecv<0)goto disconnect;//接收失败或空则断开连接
                    loginStr=RecvBuf;
                    if(loginStr.find("ProxyPort2-Login ")==0){//ip登录密码验证
                        vector<string> LoginArr;
                        LoginArr=split(loginStr,' ');
                        if(LoginArr.size()==4){
                            //LoginArr[3]
                            string pwd;
                            pwd="ProxyPort2_";
                            pwd+=ipHash;
                            pwd+="|";
                            pwd+=config.Auth_pwd;
                            md5.reset();
                            md5.update(pwd);
                            //printf("Debug2:%s %s %s|\n",pwd.c_str(),md5.toString().c_str(),loginStr.c_str());
                            if(md5.toString()==LoginArr[3]){
                                log_tmp="登录成功来自";
                                log_tmp+=client_addr;
                                wlog(log_tmp);
                                printf("登录成功来自%s\n",client_addr.c_str());
                                allowIP.push_back(inet_ntoa(client_sockaddr.sin_addr));
                                send(ClientSock, "1", 1, 0);
                            }else{
                                //printf("1:%d\n2:%d\n",pwd.size(),LoginArr[3].size());

                                log_tmp="登录失败来自";
                                log_tmp+=client_addr;
                                wlog(log_tmp);
                                printf("登录失败来自%s\n",client_addr.c_str());
                                send(ClientSock, "0", 1, 0);
                            }
                        }
                    }
                    goto disconnect;
                }
                if(nRecv > 0){
                    isNeedSend=true;
                    NeedSendLength=nRecv;
                    memcpy(NeedSendStr,RecvBuf,sizeof(RecvBuf));
                }
                if(config.is_Auth&&!verify){//如果需要授权
                    //且未登录，则转自未授权主机端口，不验证也不test
                    if(config.unAuthPort<=0)goto disconnect;
                    connect_host=config.unAuthHost==""?"127.0.0.1":config.unAuthHost;
                    connect_port=config.unAuthPort;
                    goto transport;
                }
                if(config.is_Auth&&nRecv > 0&&verify){
                    //已经登录了
                    if(loginStr=="ProxyPort2-Login"){
                        send(ClientSock, "ProxyPort2-success", 18, 0);
                        goto disconnect;
                    }
                }
                if(nRecv <= 0){
                    isNeedRecv=true;
                    int sendStrSize=config.sendStr.size();
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
                                goto disconnect;
                            }
                        }else break;
                    }
                }
                if (nRecv <= 0){
                    if(config.debug>2)printf("客户端类型未识别！已断开！\n");
                    log_tmp=client_addr;
                    log_tmp+="未识别的客户端类型";
                    wlog(log_tmp);
                    goto disconnect;
                }

                if(config.debug>2)printf("收到：%s\n",RecvBuf);
                int nn=proxyRule.size();


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
                    goto disconnect;
                }
            }else{//那就是无条件转发模式
                if(config.forwardHost=="")connect_host="127.0.0.1";else connect_host=config.forwardHost;
                connect_port=config.forwardPort;
            }
transport:
            if(config.debug>2)printf("转发成功：%s:%d\n",connect_host.c_str(),connect_port);
            log_tmp=client_addr;
            log_tmp+="=>";
            log_tmp+=connect_host;
            log_tmp+=":";
            log_tmp+=to_string(connect_port);
            wlog(log_tmp);
			ServerSock = socket(AF_INET,SOCK_STREAM,0);
			if(ServerSock == INVALID_SOCKET){
				if(config.debug>2)printf("socket链接错误！%d\n",WSAGetLastError());
				break;//客户端断开
			}
 			pt = gethostbyname(connect_host.c_str());//解析域名IP
			if(!pt)break;//客户端断开
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
			sockaddr.sin_port = htons(connect_port);//设置要连接的IP和端口
			int ret;
			ret=connect(ServerSock, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if(ret == SOCKET_ERROR)break;//客户端断开
			if(ServerSock <= 0){
				if(config.debug>2)printf("socket connect失败！%d\n",WSAGetLastError());
				break;//客户端断开
			}
			if(isNeedRecv){
                char RecvBuf2[MAXBUFSIZE];
                recv(ServerSock,RecvBuf2, sizeof(RecvBuf2), 0);
                send(ServerSock,RecvBuf,nRecv,0);
                //printf("Debug: recv ok!");
			}
			if(isNeedSend){
                send(ServerSock,NeedSendStr,NeedSendLength,0);
                //printf("Debug: send ok!");
			}

            //if(config.forwardPort==0){//说明不是无条件发送
            //
            //}
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
disconnect:
            log_tmp=client_addr;
            log_tmp+="连接断开！";
            wlog(log_tmp);
            if(config.debug>1)printf("客户端断开，线程已就绪\n");
			closesocket(ClientSock);
			delete task->client;
			task->client=NULL;
			task->ready=true;
			SuspendThread(task->handle);
		}
		//delay(5);//降低cpu
	}
	CloseHandle(task->handle);
    return 0L;
}
DWORD WINAPI Main(LPVOID lpParamter){
    return 0L;
}
int createNewTaskThread(ChildType* t){
	t->index=taskThreadList.size();
	taskThreadList.push_back(t);
	HANDLE hThread = CreateThread(NULL, 0, Child, t, 0, NULL);//启动子线程
	t->handle=hThread;

    //CloseHandle(hThread);

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
			ResumeThread(taskThreadList[i]->handle);
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
    access_log.open(path+"\\access.log", ios::app);
    if(access_log.fail()){
        printf("Warn: 日志文件打开失败！\n");
        return false;
    }
    wlog("ProxyPort初始化中...");
    ch_conf conf(path+"\\portProxy.conf");
    ch_conf conf2(path+"\\portType.conf");

    //access_log<<"123123\r\n";

    //access_log<<"gg66\n";//<<endl;
    //access_log.flush();
    //access_log<<;

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
    //设置无条件转发端口
    confstr=conf.getKeyValue("general","forwardPort");
    if(confstr!=""){
        if(!is_numeric(confstr)){
            printf("Error: %s无条件转发端口号必须是数字！",confstr.c_str());
            return false;
        }
        if(atoi(confstr.c_str())<1||atoi(confstr.c_str())>65535){
            printf("Error: %s无条件转发端口号只能在1到65535之间！",confstr.c_str());
            return false;
        }
        config.forwardPort=atoi(confstr.c_str());//设置无条件转发端口号
    }else config.forwardPort=0;
    confstr=conf.getKeyValue("general","forwardHost");
    if(confstr=="")
        config.forwardHost="";
    else
        config.forwardHost=confstr;//设置端口号

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
        SuspendThread(t->handle);
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
    //设置授权设置
    confstr=conf.getKeyValue("general","authPwd");
    if(confstr==""){
        config.is_Auth=false;
    }else{
        config.is_Auth=true;
        config.Auth_pwd=confstr;
        config.unAuthHost=conf.getKeyValue("general","unAuthHost");
        config.unAuthPort=atoi(conf.getKeyValue("general","unAuthPort").c_str());
    }


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
    wlog("ProxyPort初始化成功！");
    return true;
}
void init_WSA(){
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		printf("Error: socket初始化失败！\n");
		return;
	};//初始化
}
void init_socket(){
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
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
	wlog("ProxyPort启动成功！");
    while(1){
		/*SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//等待客户端连接 阻塞
		SOCKET *client = new SOCKET;
		*client=client_tmp;*/
		SOCKET *client = new SOCKET;
		*client = accept(s, NULL, NULL);
		struct sockaddr_in sa;
        int len = sizeof(sa);
        string addr;
        if(!getpeername(*client, (struct sockaddr *)&sa, &len)){
            addr=inet_ntoa(sa.sin_addr);
            addr+=":";
            addr+=to_string(ntohs(sa.sin_port));
            addr+="接入";
        }
		wlog(addr);
		sendTask(client);//分配线程任务
		if(config.debug>1)printf("分配线程！\n");
	}
    closesocket(s);
    WSACleanup();
}
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType){
    if(CTRL_CLOSE_EVENT == dwCtrlType){// 控制台将要被关闭，这里添加你的处理代码 ...
        wlog("正常关闭！");
    }
    return true;
}
void login(string host,int port,string authPwd){
    SOCKET ServerSock;
    string SendStr;
    ServerSock = socket(AF_INET,SOCK_STREAM,0);
    if(ServerSock == INVALID_SOCKET){
        printf("fail\n");
        return;//客户端断开
    }
    struct hostent*pt=NULL;
    pt = gethostbyname(host.c_str());//解析域名IP
    if(!pt)return;//客户端断开
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
    sockaddr.sin_port = htons(port);//设置要连接的IP和端口
    int ret;
    ret=connect(ServerSock, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
    if(ret == SOCKET_ERROR)return;//客户端断开
    if(ServerSock <= 0){
        return;//客户端断开
    }
    struct sockaddr_in listendAddr;
    int listendAddrLen;
    listendAddrLen = sizeof(listendAddr);
    if(getsockname(ServerSock, (struct sockaddr *)&listendAddr, &listendAddrLen) == -1){
        printf("getsockname error\n");
        return;
    }
    //printf("listen address = %s:%d\n", , ntohs(listendAddr.sin_port));

    MD5 md5;
    send(ServerSock,"ProxyPort2-Login",strlen("ProxyPort2-Login"),0);
    char RecvBuf[MAXBUFSIZE];
    memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
    int nRecv;
    nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
    if(nRecv<0){
        printf("登录未返回数据！");
        return;
    }
    string ipHash=RecvBuf;
    if(nRecv==18&&ipHash.find("ProxyPort2-success")==0){
        printf("已经登录过了，不要重复登录哦！");
        return;
    }
    if(nRecv!=32){
        printf("已经登录成功了或协议不正确！");
        return;
    }

    string pwd;
    pwd="ProxyPort2_";
    pwd+=ipHash;
    pwd+="|";
    pwd+=authPwd;

    md5.update(pwd);
    SendStr="ProxyPort2-Login ";
    SendStr+=host+" ";
    SendStr+=to_string(port)+" ";
    SendStr+=md5.toString();
    //printf("Debug1:%s %s|\n",pwd.c_str(),md5.toString().c_str());
    send(ServerSock,SendStr.c_str(),SendStr.size(),0);
    memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
    nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
    string result;
    result=RecvBuf;
    if(result=="1"){
        printf("登录成功！欢迎登录！\n");
    }else if(result=="0"){
        printf("登录失败！密码错误！\n");
    }else{
        printf("登录失败！返回值：%s\n",result.c_str());
    }
}

int main(int argc,char *argv[]){

    init_WSA();
	if(argc>1){
        printf("ProxyPort: 收到启动参数...\n");
	    string type;
        type=argv[1];
        if(type=="-l"){//login
            if(argc==5){
                string host,pwd;
                int port;
                host=argv[2];
                port=stoi(argv[3]);
                pwd=argv[4];
                printf("ProxyPort: 登录中 %s %d...\n",host.c_str(),port);
                login(host,port,pwd);
                /*
                argv[0]//-l  login
                argv[1]//yiban.glut.edu.cn    host
                argv[2]//80                   port
                argv[3]//pwd                  pwd
                */
            }
        }
        printf("End\n");
        return 0;
	}
	printf("ProxyPort: 启动中...\n");
	printf("-----CH制作 2020-02-02\n");
    if(!init_config())return 1;//初始化配置文件
    if(SetConsoleCtrlHandler(HandlerRoutine, TRUE)){

    }
    init_socket();
    return 0;
}
