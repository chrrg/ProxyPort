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
typedef struct childDataType
{
    int index;
	SOCKET* client;
    bool ready;//����״̬
}ChildType;
vector<ChildType *>  taskThreadList;//�����������

/*typedef struct conf1{
    string type;
    vector<regex> reg;
}PortType;
typedef struct conf2{
    int port;
    regex reg;
}PortProxy;
vector<PortType> portType;
vector<PortProxy> portProxy;
char portType_Path[MAX_PATH + 1]={0};
char portProxy_Path[MAX_PATH + 1]={0};
*/
/*bool init_portType(){
    fstream fin;
    fin.open(portType_Path, ios::in);
    string tmp;
    PortType pt;
    //vector<string> string1;
    bool collectLine=false;
    int line=0;
    while(getline(fin, tmp)){//����ÿһ��
        line++;
        //v.push_back(tmp);
        if(tmp.length()==0)continue;
        bool start=false;
        bool collect=false;
        int column=0;
        string text="";
        for (auto ch : tmp){//����ÿһ���ַ�
            column++;
            if(!start){
                if(isspace(ch))continue;
                if(ch==';')break;//��һ����ע��
                if(ch!='['){
                    if(!collectLine){
                        printf("%d:%d ��ʽ����ȱ���������š�\n",line,column);
                        return false;
                    }else{
                        //�ռ��ɹ�
                        regex reg(tmp);
                        pt.reg.push_back(reg);
                        break;
                    }
                }
                start=true;//��ʼ��
            }
            if(ch=='['){//���հ׷������ַ�
                if(collect){
                    printf("%d:%d ��ʽ�����ظ����������š�\n",line,column);
                    return false;
                }
                if(pt.type.size()!=0&&pt.reg.size()!=0){
                    portType.push_back(pt);
                }
                PortType pt_new;
                pt=pt_new;//���
                collect=true;
                continue;
            }
            if(ch==']'){
                collect=false;
                if(text.size()<=0){
                    printf("%d:%d ��ʽ���󣡱�ʶ����Ϊ�ա�\n",line,column);
                    return false;
                }
                pt.type=text;
                collectLine=true;
                //cout<<"text:"<<text<<endl;
                continue;
            }
            if(collect){
                text+=ch;
                continue;
            }
            printf("%d:%d ��ʽ����\n",line,column);
            return false;
        }
        //��������
        if(collect){//�����ռ�
            printf("%d:%d ��ʽ����δ�����ĸ�ʽ��\n",line,column);
            return false;
        }
    }
    fin.close();
}
bool init_portProxy(){
    fstream fin;
    fin.open(portProxy_Path, ios::in);
    string tmp;
    portProxy pt;
    //vector<string> string1;
    bool collectLine=false;
    int line=0;
    while(getline(fin, tmp)){//����ÿһ��
        line++;
        //v.push_back(tmp);
        if(tmp.length()==0)continue;
        bool start=false;
        bool collect=false;
        bool collect2=false;
        bool is_general=false;
        int column=0;
        string text="";
        string text2="";
        for (auto ch : tmp){//����ÿһ���ַ�
            column++;
            if(!start){
                if(isspace(ch))continue;
                if(ch==';')break;//��һ����ע��
                if(ch!='['){
                    if(!collectLine){
                        printf("%d:%d ��ʽ����ȱ���������š�\n",line,column);
                        return false;
                    }else{
                        //�ռ��ɹ�
                        regex reg(tmp);
                        pt.reg.push_back(reg);
                        break;
                    }
                    if(is_general){
                        break;
                    }
                }
                start=true;//��ʼ��
            }
            if(ch=='['){//���հ׷������ַ�
                if(collect){
                    printf("%d:%d ��ʽ�����ظ����������š�\n",line,column);
                    return false;
                }
                if(pt.type.size()!=0&&pt.reg.size()!=0){
                    portType.push_back(pt);
                }
                PortType pt_new;
                pt=pt_new;//���
                collect=true;
                continue;
            }
            if(ch==']'){
                collect=false;
                collect2=true;
                if(text.size()<=0){
                    printf("%d:%d ��ʽ���󣡱�ʶ����Ϊ�ա�\n",line,column);
                    return false;
                }
                //string general_text="general";
                is_general=text=="general";

                pt.type=text;
                //collectLine=true;
                //cout<<"text:"<<text<<endl;
                continue;
            }
            if(collect){
                text+=ch;
                continue;
            }
            if(collect2){
                text2+=ch;
                continue;
            }
            printf("%d:%d ��ʽ����\n",line,column);
            return false;
        }
        if(is_general){
            //����kv

        }
        //��������
        if(collect){//�����ռ�
            printf("%d:%d ��ʽ����ȱ���������š�\n",line,column);
            return false;
        }
        if(collect2){
            //�ռ����
            if(text2.size()!=0){//�����ͣ���ô׷��Ĭ�ϵ�����
                int k=portType.size();
                bool finded=false;
                for(int i = 0; i < k; i++){
                    if(strcmp(portType[i].type,text2)){//��ȣ����ҵ���
                        finded=true;

                        break;
                    }
                }
                if(!finded){
                    printf("%d:%d �������󣡲����ڡ�%s�������\n",line,column,text2);
                    return false;
                }
            }
        }
        //text
        //string1.push_back(tmp);
        //printf("%c",tmp[0]);
        //cout<<"line:"<<tmp<<endl;
    }
    fin.close();

}*/
/*
bool initConfig(){
    portType_Path[0]='\0';//memset(portType_Path,'\0',sizeof(portType_Path));
    portProxy_Path[0]='\0';//memset(portProxy_Path,'\0',sizeof(portProxy_Path));
    GetModuleFileNameA(NULL, portType_Path, MAX_PATH);
    (strrchr(portType_Path, '\\'))[0] = 0; // ɾ���ļ�����ֻ���·���ִ�
    strcpy(portProxy_Path,portType_Path);
    strcat(portType_Path,"\\portType.conf");
    strcat(portProxy_Path,"\\portProxy.conf");
    ///////////////////////////////////////////////////////////////�ַ���׼�����
//�Ƚ���portType.conf
    if(!init_portType())return false;
//��ʼ����portProxy.conf
    if(!init_portProxy())return false;

    //string1.swap(vector<string>());
    / *cout << portProxy_Path << endl;
    fstream fin;
    fin.open(portProxy_Path, ios::in);
    while(getline(fin, tmp)){
        //v.push_back(tmp);
        cout<<"line:"<<tmp<<endl;
    }
}
    return false;
}
*/
void delay(int ms){
	HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	WaitForSingleObject(hEvent,ms);
}
int DataSend(SOCKET s, char * DataBuf, int DataLen){ //��DataBuf�е�DataLen���ֽڷ���sȥ
    int nBytesLeft = DataLen;
    int nBytesSent = 0;
    int ret;
    //set socket to blocking mode
    int iMode = 0;
    ioctlsocket(s, FIONBIO, (u_long FAR * ) & iMode);
    while (nBytesLeft > 0) {
        ret = send(s, DataBuf + nBytesSent, nBytesLeft, 0);
        if (ret <= 0) break;
        nBytesSent += ret;
        nBytesLeft -= ret;
    }
    return nBytesSent;
}
DWORD WINAPI Child(LPVOID lpParamter){


	//ChildType *task = (ChildType *)lpParamter;
	ChildType* task = (ChildType *)lpParamter;
	//printf("���߳�%i������ɣ�\n",task->index);
	//cout<<"thread||client:"<<task<<"||"<<task->client<<endl;
	while(1){
		//recv(s,buffer,100,NULL);
		/*while(1){
			int k=recv(s,response,sizeof(response),0);
			if(SOCKET_ERROR == k){
				int err = WSAGetLastError();
				if(err == WSAEWOULDBLOCK){//������
					continue;
				}else if(err == WSAETIMEDOUT || err == WSAENETDOWN){
					printf("ERROR: �ִ���!\n");
					return -1;
				}
			}
			printf("length:%d\n",k);
		}*/

		if(task->client!=NULL){

			task->ready=false;
			char buffer[4096];
			char response[sizeof(buffer)];
			//int size=sizeof(buffer);//��������С
			//int i=0;
			//bool b;

			//SOCKET s = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);//IPPROTO_TCP
			SOCKET s = socket(AF_INET,SOCK_STREAM,0);
			if (s == INVALID_SOCKET){
				printf("socket���Ӵ���%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			sockaddr.sin_port = htons(21);//����Ҫ���ӵ�IP�Ͷ˿�
			/*
			u_long is_non_block = 1;
			ioctlsocket(s,FIONBIO,&is_non_block);
			connect(s,(SOCKADDR*)&sockaddr,sizeof(SOCKADDR));//tcp����
			*/
			int ret;
			ret=connect(s, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if (ret == SOCKET_ERROR){
				//printf("socket����ʧ�ܣ�%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
			if (s <= 0) {
				printf("socket connectʧ�ܣ�%d\n",WSAGetLastError());
				break;//�ͻ��˶Ͽ�
			}
			memset(buffer,0,sizeof(buffer));//clean buffer
			memset(response,0,sizeof(response));

			SOCKET ClientSock=*task->client;
			SOCKET ServerSock=s;
			char RecvBuf[MAXBUFSIZE] = {
				0
			};
			fd_set Fd_Read;
			int nRecv;
			while(1){
				FD_ZERO( & Fd_Read);
				FD_SET(ClientSock, & Fd_Read);
				FD_SET(ServerSock, & Fd_Read);
				ret = select(0, & Fd_Read, NULL, NULL, NULL);
				if (ret <= 0){printf("selectʧ�ܣ�\n");break;}
				if (FD_ISSET(ClientSock, & Fd_Read)) {
					nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("�ͻ��˶Ͽ���\n");
						break;
					}
					//ret = DataSend(ServerSock, RecvBuf, nRecv);
					printf("ClientSock:%s\n",RecvBuf);
					send(ServerSock, RecvBuf, nRecv, 0);
				}
				if (FD_ISSET(ServerSock, & Fd_Read)) {
					nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("�ͻ��˶Ͽ���\n");break;
					}
					//ret = DataSend(ClientSock, RecvBuf, nRecv);
					printf("ServerSock:%s\n",RecvBuf);
					send(ClientSock, RecvBuf, nRecv, 0);
				}
			}
			closesocket(ServerSock);
			closesocket(ClientSock);
			task->client=NULL;
			task->ready=true;
		}
		delay(5);//����cpu
	}
    return 0L;
}
DWORD WINAPI Main(LPVOID lpParamter){
    cout<<"MainThread Success"<<endl;
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
	//std::cout<<"client2:"<<client<<"|"<<*client<<std::endl;
	//�ɷ�����
	//ѡ�̳߳�
	//printf("�ͻ���%i���ӳɹ���\n",client);
	int k=taskThreadList.size();
	for(int i=0;i<k;i++){
		if(taskThreadList[i]->ready){
			//printf("ָ��%iȥ�������\n",i);
			taskThreadList[i]->ready=false;
			taskThreadList[i]->client=client;
			return;
		}
    }
	ChildType* t=new ChildType;
	t->ready=false;
	t->client=client;
	createNewTaskThread(t);
}
int main(int argc,char *argv[]){
	printf("������...\n");
    ch_conf conf("D:\\�ĵ�\\codeblock\\ProxyPort2\\bin\\Debug\\portProxy.conf");
    if(!conf.verifyKeyValue("general")){
        printf("�����ļ�����ʧ�ܣ�Key-Value��ʽ����ȷ��\n");
        return -1;
    }
    cout<<"port:"<<conf.getKeyValue("general","port")<<endl;
    ch_conf conf2("D:\\�ĵ�\\codeblock\\ProxyPort2\\bin\\Debug\\portType.conf");

    //cout<<"port:"<<conf.getKeyValue("general","port")<<endl;

    //if(!initConfig()){printf("�����ļ�����ʧ�ܣ����������ļ���\n");return -1;}

    return 0;
	//���������ļ�
	/*string fnames[] = {"foo.txt", "bar.txt", "test", "a0.txt", "AAA.txt"};
    regex txt_regex("[a-z]+\\.txt");
    for (const auto &fname: fnames){
        cout << fname << ": " << regex_match(fname, txt_regex) << endl;
    }*/

	int PORT=5000;
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		cout << "socket��ʼ��ʧ�ܣ�\n" << endl;
		return -1;
	};//��ʼ��
	SOCKET  s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET){// == SOCKET_ERROR
		cout << "socketִ��ʧ�ܣ�\n" << endl;
		return -1;
	}
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("0.0.0.0");
    sockaddr.sin_port = htons(PORT);
	int n;
	n=bind(s,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in));
	if (n==SOCKET_ERROR) {// == SOCKET_ERROR
		cout << "�˿�"<<PORT<<"��ʧ�ܣ�" << endl;
		return -1;
	}else {
		cout << "�˿ڰ󶨳ɹ���" << PORT << endl;
	}
    n=listen(s,3);//��ʼ����//1
	if (n<0) {// == SOCKET_ERROR
		cout << "����ʧ�ܣ�" << endl;
		return -1;
	}
	//int nsize = sizeof(SOCKADDR);
	printf("�����ɹ���\n");
    while(1){
		SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//�ȴ��ͻ������� ����
		if(INVALID_SOCKET == client_tmp){
			int err = WSAGetLastError();
			if(err == WSAEWOULDBLOCK){
				//delay(5);
				continue;
			}else{
				printf("accept failed!\n");
				closesocket(s);
				WSACleanup();
				return -1;
			}
		}
		SOCKET *client = new SOCKET;
		*client=client_tmp;
		sendTask(client);//�����߳�����
	}
    closesocket(s);

    WSACleanup();
    return 0;
}
