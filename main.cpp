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
    bool ready=false;//����״̬
}ChildType;
vector<ChildType *> taskThreadList;//�����������
typedef struct proxyRuleType{
    int port;
    string host;
	vector<regex> reg;
}ProxyRule;
vector<ProxyRule> proxyRule;//ת������
typedef struct proxyPort_Config{
    vector<string> sendStr;
    int port=0;
    int debug=0;
    int initThread=0;
    int timeout;
}proxyPortConfig;
proxyPortConfig config;

void delay(int ms){HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);WaitForSingleObject(hEvent,ms);}//��ʱ����

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
	if(config.debug>2)printf("�߳�%d��������\n",task->index);
	while(1){
		if(task->client!=NULL){
            task->ready=false;
            if(config.debug>1)printf("�߳�%d�յ���\n",task->index);
            SOCKET ClientSock=*task->client;
            int timeout = config.timeout;
            setsockopt(ClientSock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
            char RecvBuf[MAXBUFSIZE];
            memset(RecvBuf,0,sizeof(RecvBuf));//clean buffer
            int nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
            if(config.debug>10)printf("recv���س��ȣ�%d�����ݣ�%s|||\n",nRecv,HexToAscii((unsigned char*)RecvBuf,nRecv).c_str());

            int sendStrSize=config.sendStr.size();
            if(nRecv <= 0)
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
                            closesocket(ClientSock);
                            task->client=NULL;
                            task->ready=true;
                            continue;
                        }
                    }else break;
                }
            if (nRecv <= 0){
                if(config.debug>2)printf("�ͻ�������δʶ���ѶϿ���\n");
                closesocket(ClientSock);
                task->client=NULL;
                task->ready=true;
                continue;
            }

            if(config.debug>2)printf("�յ���%s\n",RecvBuf);
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
                if(config.debug>2)printf("δ֪���ͣ��յ���%s|\n",RecvBuf);
                closesocket(ClientSock);
                task->client=NULL;
                task->ready=true;
                continue;
            }
            if(config.debug>2)printf("ת���ɹ���%s:%d\n",connect_host.c_str(),connect_port);
			SOCKET s = socket(AF_INET,SOCK_STREAM,0);
			if(s == INVALID_SOCKET){
				if(config.debug>2)printf("socket���Ӵ���%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
			struct hostent *pt = gethostbyname(connect_host.c_str());//��������IP
			if(!pt)break;//�ͻ��˶Ͽ�
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			memcpy(&sockaddr.sin_addr, pt->h_addr, 4);
			sockaddr.sin_port = htons(connect_port);//����Ҫ���ӵ�IP�Ͷ˿�
			int ret;
			ret=connect(s, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if(ret == SOCKET_ERROR)break;//�ͻ��˶Ͽ�
			if(s <= 0){
				if(config.debug>2)printf("socket connectʧ�ܣ�%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
			SOCKET ServerSock=s;
            send(ServerSock,RecvBuf,nRecv,0);
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
			closesocket(ClientSock);
			task->client=NULL;
			task->ready=true;
			if(config.debug>1)printf("�ͻ��˶Ͽ����߳��Ѿ���\n");
		}
		delay(5);//����cpu
	}
    return 0L;
}
DWORD WINAPI Main(LPVOID lpParamter){
    return 0L;
}
int createNewTaskThread(ChildType* t){
	t->index=taskThreadList.size();
	taskThreadList.push_back(t);
	HANDLE hThread = CreateThread(NULL, 0, Child, t, 0, NULL);//�������߳�
    CloseHandle(hThread);
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
    ch_conf conf(path+"\\portProxy.conf");
    ch_conf conf2(path+"\\portType.conf");
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
    return true;
}
void init_socket(){
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		printf("Error: socket��ʼ��ʧ�ܣ�\n");
		return;
	};//��ʼ��
	SOCKET  s = socket(AF_INET, SOCK_STREAM, 0);
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
    while(1){
		SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//�ȴ��ͻ������� ����
		SOCKET *client = new SOCKET;
		*client=client_tmp;
		sendTask(client);//�����߳�����
		if(config.debug>1)printf("�����̣߳�\n");
	}
    closesocket(s);
    WSACleanup();
}
int main(int argc,char *argv[]){
	printf("ProxyPort: ������...\n");
    if(!init_config())return 1;//��ʼ�������ļ�
    init_socket();
    return 0;
}