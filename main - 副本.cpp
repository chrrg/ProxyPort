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
    bool ready;//就绪状态
}ChildType;
vector<ChildType *>  taskThreadList;//任务进程链表

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
    while(getline(fin, tmp)){//遍历每一行
        line++;
        //v.push_back(tmp);
        if(tmp.length()==0)continue;
        bool start=false;
        bool collect=false;
        int column=0;
        string text="";
        for (auto ch : tmp){//遍历每一个字符
            column++;
            if(!start){
                if(isspace(ch))continue;
                if(ch==';')break;//这一行是注释
                if(ch!='['){
                    if(!collectLine){
                        printf("%d:%d 格式错误！缺少左中括号。\n",line,column);
                        return false;
                    }else{
                        //收集成功
                        regex reg(tmp);
                        pt.reg.push_back(reg);
                        break;
                    }
                }
                start=true;//开始了
            }
            if(ch=='['){//除空白符的首字符
                if(collect){
                    printf("%d:%d 格式错误！重复的左中括号。\n",line,column);
                    return false;
                }
                if(pt.type.size()!=0&&pt.reg.size()!=0){
                    portType.push_back(pt);
                }
                PortType pt_new;
                pt=pt_new;//清空
                collect=true;
                continue;
            }
            if(ch==']'){
                collect=false;
                if(text.size()<=0){
                    printf("%d:%d 格式错误！标识不能为空。\n",line,column);
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
            printf("%d:%d 格式错误！\n",line,column);
            return false;
        }
        //遍历完了
        if(collect){//还在收集
            printf("%d:%d 格式错误！未结束的格式。\n",line,column);
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
    while(getline(fin, tmp)){//遍历每一行
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
        for (auto ch : tmp){//遍历每一个字符
            column++;
            if(!start){
                if(isspace(ch))continue;
                if(ch==';')break;//这一行是注释
                if(ch!='['){
                    if(!collectLine){
                        printf("%d:%d 格式错误！缺少左中括号。\n",line,column);
                        return false;
                    }else{
                        //收集成功
                        regex reg(tmp);
                        pt.reg.push_back(reg);
                        break;
                    }
                    if(is_general){
                        break;
                    }
                }
                start=true;//开始了
            }
            if(ch=='['){//除空白符的首字符
                if(collect){
                    printf("%d:%d 格式错误！重复的左中括号。\n",line,column);
                    return false;
                }
                if(pt.type.size()!=0&&pt.reg.size()!=0){
                    portType.push_back(pt);
                }
                PortType pt_new;
                pt=pt_new;//清空
                collect=true;
                continue;
            }
            if(ch==']'){
                collect=false;
                collect2=true;
                if(text.size()<=0){
                    printf("%d:%d 格式错误！标识不能为空。\n",line,column);
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
            printf("%d:%d 格式错误！\n",line,column);
            return false;
        }
        if(is_general){
            //解析kv

        }
        //遍历完了
        if(collect){//还在收集
            printf("%d:%d 格式错误！缺少右中括号。\n",line,column);
            return false;
        }
        if(collect2){
            //收集完成
            if(text2.size()!=0){//有类型，那么追加默认的正则
                int k=portType.size();
                bool finded=false;
                for(int i = 0; i < k; i++){
                    if(strcmp(portType[i].type,text2)){//相等，就找到了
                        finded=true;

                        break;
                    }
                }
                if(!finded){
                    printf("%d:%d 配置有误！不存在【%s】配置项。\n",line,column,text2);
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
    (strrchr(portType_Path, '\\'))[0] = 0; // 删除文件名，只获得路径字串
    strcpy(portProxy_Path,portType_Path);
    strcat(portType_Path,"\\portType.conf");
    strcat(portProxy_Path,"\\portProxy.conf");
    ///////////////////////////////////////////////////////////////字符串准备完毕
//先解析portType.conf
    if(!init_portType())return false;
//开始解析portProxy.conf
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
int DataSend(SOCKET s, char * DataBuf, int DataLen){ //将DataBuf中的DataLen个字节发到s去
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
	//printf("新线程%i创建完成！\n",task->index);
	//cout<<"thread||client:"<<task<<"||"<<task->client<<endl;
	while(1){
		//recv(s,buffer,100,NULL);
		/*while(1){
			int k=recv(s,response,sizeof(response),0);
			if(SOCKET_ERROR == k){
				int err = WSAGetLastError();
				if(err == WSAEWOULDBLOCK){//非阻塞
					continue;
				}else if(err == WSAETIMEDOUT || err == WSAENETDOWN){
					printf("ERROR: 粗错了!\n");
					return -1;
				}
			}
			printf("length:%d\n",k);
		}*/

		if(task->client!=NULL){

			task->ready=false;
			char buffer[4096];
			char response[sizeof(buffer)];
			//int size=sizeof(buffer);//缓冲区大小
			//int i=0;
			//bool b;

			//SOCKET s = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);//IPPROTO_TCP
			SOCKET s = socket(AF_INET,SOCK_STREAM,0);
			if (s == INVALID_SOCKET){
				printf("socket链接错误！%d\n",WSAGetLastError());
				break;//客户端断开
			}
			struct sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;//PF_INET;
			sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			sockaddr.sin_port = htons(21);//设置要连接的IP和端口
			/*
			u_long is_non_block = 1;
			ioctlsocket(s,FIONBIO,&is_non_block);
			connect(s,(SOCKADDR*)&sockaddr,sizeof(SOCKADDR));//tcp连接
			*/
			int ret;
			ret=connect(s, (struct sockaddr * ) & sockaddr, sizeof(struct sockaddr_in));
			if (ret == SOCKET_ERROR){
				//printf("socket链接失败！%d\n",WSAGetLastError());
				break;//客户端断开
			}
			if (s <= 0) {
				printf("socket connect失败！%d\n",WSAGetLastError());
				break;//客户端断开
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
				if (ret <= 0){printf("select失败！\n");break;}
				if (FD_ISSET(ClientSock, & Fd_Read)) {
					nRecv = recv(ClientSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("客户端断开！\n");
						break;
					}
					//ret = DataSend(ServerSock, RecvBuf, nRecv);
					printf("ClientSock:%s\n",RecvBuf);
					send(ServerSock, RecvBuf, nRecv, 0);
				}
				if (FD_ISSET(ServerSock, & Fd_Read)) {
					nRecv = recv(ServerSock, RecvBuf, sizeof(RecvBuf), 0);
					if (nRecv <= 0){
						//printf("客户端断开！\n");break;
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
		delay(5);//降低cpu
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
	HANDLE hThread = CreateThread(NULL, 0, Child, t, 0, NULL);//启动子线程
    CloseHandle(hThread);
	return 1;
}
void sendTask(SOCKET* client){
	//std::cout<<"client2:"<<client<<"|"<<*client<<std::endl;
	//派发数据
	//选线程池
	//printf("客户端%i链接成功！\n",client);
	int k=taskThreadList.size();
	for(int i=0;i<k;i++){
		if(taskThreadList[i]->ready){
			//printf("指派%i去完成任务！\n",i);
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
	printf("启动中...\n");
    ch_conf conf("D:\\文档\\codeblock\\ProxyPort2\\bin\\Debug\\portProxy.conf");
    if(!conf.verifyKeyValue("general")){
        printf("配置文件加载失败！Key-Value格式不正确！\n");
        return -1;
    }
    cout<<"port:"<<conf.getKeyValue("general","port")<<endl;
    ch_conf conf2("D:\\文档\\codeblock\\ProxyPort2\\bin\\Debug\\portType.conf");

    //cout<<"port:"<<conf.getKeyValue("general","port")<<endl;

    //if(!initConfig()){printf("配置文件加载失败！请检查配置文件！\n");return -1;}

    return 0;
	//加载配置文件
	/*string fnames[] = {"foo.txt", "bar.txt", "test", "a0.txt", "AAA.txt"};
    regex txt_regex("[a-z]+\\.txt");
    for (const auto &fname: fnames){
        cout << fname << ": " << regex_match(fname, txt_regex) << endl;
    }*/

	int PORT=5000;
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
		cout << "socket初始化失败！\n" << endl;
		return -1;
	};//初始化
	SOCKET  s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET){// == SOCKET_ERROR
		cout << "socket执行失败！\n" << endl;
		return -1;
	}
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;//PF_INET;
    sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("0.0.0.0");
    sockaddr.sin_port = htons(PORT);
	int n;
	n=bind(s,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_in));
	if (n==SOCKET_ERROR) {// == SOCKET_ERROR
		cout << "端口"<<PORT<<"绑定失败！" << endl;
		return -1;
	}else {
		cout << "端口绑定成功：" << PORT << endl;
	}
    n=listen(s,3);//开始监听//1
	if (n<0) {// == SOCKET_ERROR
		cout << "监听失败！" << endl;
		return -1;
	}
	//int nsize = sizeof(SOCKADDR);
	printf("启动成功！\n");
    while(1){
		SOCKET client_tmp;// = new SOCKET;
		client_tmp = accept(s, NULL, NULL);//等待客户端连接 阻塞
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
		sendTask(client);//分配线程任务
	}
    closesocket(s);

    WSACleanup();
    return 0;
}
