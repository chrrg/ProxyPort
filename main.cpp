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
    bool ready=false;//����״̬
}ChildType;
vector<ChildType *> taskThreadList;//�����������
typedef struct proxyRuleType{
    int port;
    string host;
	vector<regex> reg;
}ProxyRule;
vector<ProxyRule> proxyRule;//ת������
vector<string> allowIP;//ת������
typedef struct proxyPort_Config{
    vector<string> sendStr;
    int port=0;
    int debug=0;
    int initThread=0;
    int timeout;
    string forwardHost="";
    int forwardPort=0;
    bool is_Auth;//�Ƿ���Ҫ��Ȩ
    string Auth_pwd;//��Ȩ����
    string unAuthHost;//δ��Ȩת������
    int unAuthPort;//δ��Ȩת���˿�
}proxyPortConfig;
proxyPortConfig config;
fstream access_log;
void delay(int ms){HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);WaitForSingleObject(hEvent,ms);}//��ʱ����
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
        if(allowIP[i]==ip)//��������б����е�ǰ���ӵ�ip
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
	if(config.debug>2)printf("�߳�%d��������\n",task->index);
	//wlog("�߳�"+to_string(task->index)+"��������");
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
            if(config.debug>1)printf("�߳�%d�յ���\n",task->index);
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
            if(config.forwardPort==0){//������ת���˿������0 ˵����Ҫ��֤
                //˵����Ҫ��Ȩ���ܷ���
                nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
                if(config.debug>10)printf("recv���س��ȣ�%d�����ݣ�%s|||\n",nRecv,HexToAscii((unsigned char*)RecvBuf,nRecv).c_str());
                //�Ƿ���Ҫ��Ȩ �һ�ȡ��������δ��֤
                bool verify=isLogin(inet_ntoa(client_sockaddr.sin_addr));
                string loginStr=RecvBuf;
                if(config.is_Auth&&nRecv > 0&&!verify&&loginStr=="ProxyPort2-Login"){//��Ҫ��Ȩ����δ��¼����Ҫ�ж��Ƿ��ǵ�¼
                    MD5 md5;
                    string ipHash;
                    md5.update("ProxyPort2_IP_"+client_addr);
                    ipHash=md5.toString();
                    //printf("loginStr:%s\n",loginStr.c_str());
                    send(ClientSock, ipHash.c_str(), ipHash.size(), 0);
                    nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
                    if(nRecv<0)goto disconnect;//����ʧ�ܻ����Ͽ�����
                    loginStr=RecvBuf;
                    if(loginStr.find("ProxyPort2-Login ")==0){//ip��¼������֤
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
                                log_tmp="��¼�ɹ�����";
                                log_tmp+=client_addr;
                                wlog(log_tmp);
                                printf("��¼�ɹ�����%s\n",client_addr.c_str());
                                allowIP.push_back(inet_ntoa(client_sockaddr.sin_addr));
                                send(ClientSock, "1", 1, 0);
                            }else{
                                //printf("1:%d\n2:%d\n",pwd.size(),LoginArr[3].size());

                                log_tmp="��¼ʧ������";
                                log_tmp+=client_addr;
                                wlog(log_tmp);
                                printf("��¼ʧ������%s\n",client_addr.c_str());
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
                if(config.is_Auth&&!verify){//�����Ҫ��Ȩ
                    //��δ��¼����ת��δ��Ȩ�����˿ڣ�����֤Ҳ��test
                    if(config.unAuthPort<=0)goto disconnect;
                    connect_host=config.unAuthHost==""?"127.0.0.1":config.unAuthHost;
                    connect_port=config.unAuthPort;
                    goto transport;
                }
                if(config.is_Auth&&nRecv > 0&&verify){
                    //�Ѿ���¼��
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
                                if(config.debug>2)printf("���ͳ��Է��ͣ�%s\n",config.sendStr[i].c_str());
                                memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
                                nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);//�������ͼ�������
                            }else{
                                if(config.debug>2)printf("�ͻ��˶Ͽ���\n");
                                goto disconnect;
                            }
                        }else break;
                    }
                }
                if (nRecv <= 0){
                    if(config.debug>2)printf("�ͻ�������δʶ���ѶϿ���\n");
                    log_tmp=client_addr;
                    log_tmp+="δʶ��Ŀͻ�������";
                    wlog(log_tmp);
                    goto disconnect;
                }

                if(config.debug>2)printf("�յ���%s\n",RecvBuf);
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
                    if(config.debug>2)printf("δ֪���ͣ��յ���%s|\n",RecvBuf);
                    goto disconnect;
                }
            }else{//�Ǿ���������ת��ģʽ
                if(config.forwardHost=="")connect_host="127.0.0.1";else connect_host=config.forwardHost;
                connect_port=config.forwardPort;
            }
transport:
            if(config.debug>2)printf("ת���ɹ���%s:%d\n",connect_host.c_str(),connect_port);
            log_tmp=client_addr;
            log_tmp+="=>";
            log_tmp+=connect_host;
            log_tmp+=":";
            log_tmp+=to_string(connect_port);
            wlog(log_tmp);
			ServerSock = socket(AF_INET,SOCK_STREAM,0);
			if(ServerSock == INVALID_SOCKET){
				if(config.debug>2)printf("socket���Ӵ���%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
 			pt = gethostbyname(connect_host.c_str());//��������IP
			if(!pt)break;//�ͻ��˶Ͽ�
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
			sockaddr.sin_port = htons(connect_port);//����Ҫ���ӵ�IP�Ͷ˿�
			int ret;
			ret=connect(ServerSock, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if(ret == SOCKET_ERROR)break;//�ͻ��˶Ͽ�
			if(ServerSock <= 0){
				if(config.debug>2)printf("socket connectʧ�ܣ�%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
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

            //if(config.forwardPort==0){//˵����������������
            //
            //}
            //printf("ת����һ�����ݣ�%s\n",RecvBuf);
            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
			fd_set Fd_Read;
			while(1){
				FD_ZERO(&Fd_Read);
				FD_SET(ClientSock,&Fd_Read);
				FD_SET(ServerSock,&Fd_Read);
				ret = select(0,&Fd_Read,NULL,NULL,NULL);
				if(ret <= 0){printf("Error: selectʧ�ܣ�\n");break;}
				if(FD_ISSET(ClientSock, & Fd_Read)) {
					nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
					if(nRecv <= 0){
						//printf("�ͻ��˶Ͽ���\n");
						break;
					}
					if(config.debug>3)printf("ClientSock:%s\n",RecvBuf);
					send(ServerSock, RecvBuf, nRecv, 0);
					memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
				}
				if(FD_ISSET(ServerSock, & Fd_Read)) {
					nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("�ͻ��˶Ͽ���\n");
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
            log_tmp+="���ӶϿ���";
            wlog(log_tmp);
            if(config.debug>1)printf("�ͻ��˶Ͽ����߳��Ѿ���\n");
			closesocket(ClientSock);
			delete task->client;
			task->client=NULL;
			task->ready=true;
			SuspendThread(task->handle);
		}
		//delay(5);//����cpu
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
	HANDLE hThread = CreateThread(NULL, 0, Child, t, 0, NULL);//�������߳�
	t->handle=hThread;

    //CloseHandle(hThread);

	return 1;
}
void sendTask(SOCKET* client){
	//�ɷ�����
	//ѡ�̳߳�
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
    (strrchr(szFilePath, '\\'))[0] = 0; // ɾ���ļ�����ֻ���·���ִ�
    string path = szFilePath;
    return path;
}

bool init_config(){

    //ch_conf conf("D:\\�ĵ�\\codeblock\\ProxyPort2\\bin\\Debug\\portProxy.conf");
    //ch_conf conf2("D:\\�ĵ�\\codeblock\\ProxyPort2\\bin\\Debug\\portType.conf");
    string path=GetExePath();
    access_log.open(path+"\\access.log", ios::app);
    if(access_log.fail()){
        printf("Warn: ��־�ļ���ʧ�ܣ�\n");
        return false;
    }
    wlog("ProxyPort��ʼ����...");
    ch_conf conf(path+"\\portProxy.conf");
    ch_conf conf2(path+"\\portType.conf");

    //access_log<<"123123\r\n";

    //access_log<<"gg66\n";//<<endl;
    //access_log.flush();
    //access_log<<;

    if(!conf.verifyKeyValue("general")){
        printf("Error: �����ļ�����ʧ�ܣ�Key-Value��ʽ����ȷ��\n");
        return false;
    }
    string confstr;
    /////////////////////////////////////////////////////////////
    //���ö˿�
    confstr=conf.getKeyValue("general","port");
    if(!is_numeric(confstr)){
        printf("Error: %s�����˿ںű��������֣�",confstr.c_str());
        return false;
    }
    if(atoi(confstr.c_str())<1||atoi(confstr.c_str())>65535){
        printf("Error: %s�����˿ں�ֻ����1��65535֮�䣡",confstr.c_str());
        return false;
    }
    config.port=atoi(confstr.c_str());//���ö˿ں�
    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //����������ת���˿�
    confstr=conf.getKeyValue("general","forwardPort");
    if(confstr!=""){
        if(!is_numeric(confstr)){
            printf("Error: %s������ת���˿ںű��������֣�",confstr.c_str());
            return false;
        }
        if(atoi(confstr.c_str())<1||atoi(confstr.c_str())>65535){
            printf("Error: %s������ת���˿ں�ֻ����1��65535֮�䣡",confstr.c_str());
            return false;
        }
        config.forwardPort=atoi(confstr.c_str());//����������ת���˿ں�
    }else config.forwardPort=0;
    confstr=conf.getKeyValue("general","forwardHost");
    if(confstr=="")
        config.forwardHost="";
    else
        config.forwardHost=confstr;//���ö˿ں�

    /////////////////////////////////////////////////////////////
    //���õ���
    confstr=conf.getKeyValue("general","debug");
    if(!is_numeric(confstr)){
        printf("Error: %s���Կ��ر��������֣�",confstr.c_str());
        return false;
    }
    config.debug=atoi(confstr.c_str());
    if(config.debug>0)printf("Warn: ����ģʽ�Ѿ��򿪣�%d\n",config.debug);

    /////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////
    //������С�߳�
    confstr=conf.getKeyValue("general","initThread");
    if(!is_numeric(confstr)){
        printf("Error: %s��ʹ�̱߳��������֣�",confstr.c_str());
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
    //���ñ���ģʽ��ʱʱ��
    confstr=conf.getKeyValue("general","timeout");
    if(!is_numeric(confstr)){
        printf("Error: %s��ʱʱ����������֣�",confstr.c_str());
        return false;
    }
    config.timeout=atoi(confstr.c_str());
    /////////////////////////////////////////////////////////////
    //������Ȩ����
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
    //���ò����ı�
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

    //���ö˿�ת������


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
            printf("Warn: %d�˿ں�ֻ����1��65535֮�䣡",rule.port);
            continue;
        }
        int k=t.value.size();
        //�Զ��������
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
    wlog("ProxyPort��ʼ���ɹ���");
    return true;
}
void init_WSA(){
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		printf("Error: socket��ʼ��ʧ�ܣ�\n");
		return;
	};//��ʼ��
}
void init_socket(){
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET){// == SOCKET_ERROR
		printf("Error: socketִ��ʧ�ܣ�\n");
		return;
	}
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("0.0.0.0");
    sockaddr.sin_port = htons(config.port);
	int n;
	n=bind(s,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in));
	if (n==SOCKET_ERROR) {// == SOCKET_ERROR
		printf("Error: �˿�%d��ʧ�ܣ�\n",config.port);
		return;
	}else printf("�˿ڰ󶨳ɹ���%d\n",config.port);

    n=listen(s,3);//��ʼ����//1
	if (n<0){// == SOCKET_ERROR
		printf("Error: ����ʧ�ܣ�\n");
		return;
	}
	printf("�����ɹ���\n");
	wlog("ProxyPort�����ɹ���");
    while(1){
		/*SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//�ȴ��ͻ������� ����
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
            addr+="����";
        }
		wlog(addr);
		sendTask(client);//�����߳�����
		if(config.debug>1)printf("�����̣߳�\n");
	}
    closesocket(s);
    WSACleanup();
}
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType){
    if(CTRL_CLOSE_EVENT == dwCtrlType){// ����̨��Ҫ���رգ����������Ĵ������ ...
        wlog("�����رգ�");
    }
    return true;
}
void login(string host,int port,string authPwd){
    SOCKET ServerSock;
    string SendStr;
    ServerSock = socket(AF_INET,SOCK_STREAM,0);
    if(ServerSock == INVALID_SOCKET){
        printf("fail\n");
        return;//�ͻ��˶Ͽ�
    }
    struct hostent*pt=NULL;
    pt = gethostbyname(host.c_str());//��������IP
    if(!pt)return;//�ͻ��˶Ͽ�
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
    sockaddr.sin_port = htons(port);//����Ҫ���ӵ�IP�Ͷ˿�
    int ret;
    ret=connect(ServerSock, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
    if(ret == SOCKET_ERROR)return;//�ͻ��˶Ͽ�
    if(ServerSock <= 0){
        return;//�ͻ��˶Ͽ�
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
        printf("��¼δ�������ݣ�");
        return;
    }
    string ipHash=RecvBuf;
    if(nRecv==18&&ipHash.find("ProxyPort2-success")==0){
        printf("�Ѿ���¼���ˣ���Ҫ�ظ���¼Ŷ��");
        return;
    }
    if(nRecv!=32){
        printf("�Ѿ���¼�ɹ��˻�Э�鲻��ȷ��");
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
        printf("��¼�ɹ�����ӭ��¼��\n");
    }else if(result=="0"){
        printf("��¼ʧ�ܣ��������\n");
    }else{
        printf("��¼ʧ�ܣ�����ֵ��%s\n",result.c_str());
    }
}

int main(int argc,char *argv[]){

    init_WSA();
	if(argc>1){
        printf("ProxyPort: �յ���������...\n");
	    string type;
        type=argv[1];
        if(type=="-l"){//login
            if(argc==5){
                string host,pwd;
                int port;
                host=argv[2];
                port=stoi(argv[3]);
                pwd=argv[4];
                printf("ProxyPort: ��¼�� %s %d...\n",host.c_str(),port);
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
	printf("ProxyPort: ������...\n");
	printf("-----CH���� 2020-02-02\n");
    if(!init_config())return 1;//��ʼ�������ļ�
    if(SetConsoleCtrlHandler(HandlerRoutine, TRUE)){

    }
    init_socket();
    return 0;
}
