#include <string>
#include <vector>
#include <string>
#include <fstream>

typedef struct sub_ch_conf{
    string name;
    string extra;
    vector<string> value;
}CHConfigType;

class ch_conf{
public:
    std::vector<std::string> split(const std::string& s, char delimiter){
       std::vector<std::string> tokens;
       std::string token;
       std::istringstream tokenStream(s);
       while(std::getline(tokenStream, token, delimiter))tokens.push_back(token);
       return tokens;
    }
    void replace(std::string& str, const std::string& before, const std::string& after){
        for (std::string::size_type pos(0); pos != std::string::npos; pos += after.length()){
            pos = str.find(before, pos);
            if (pos != std::string::npos)str.replace(pos, before.length(), after);else break;
        }
    }
    vector<string> operator[](string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return all_conf[i].value;
        vector<string> null;
        return null;
    }
    CHConfigType operator[](int index){
        return all_conf[index];
    }
    CHConfigType at(int i){
        return all_conf[i];
    }
    int size(){
        return all_conf.size();
    }
    vector<string> getExtraArrSplitedBySpace(string key){
        vector<string> result;
        int k=all_conf.size();
        for(int i=0;i<k;i++)if(all_conf[i].name==key)
            return split(all_conf[i].extra,' ');
        return result;
    }
    CHConfigType* get(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return &all_conf[i];
        return NULL;
    }
    CHConfigType read(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return all_conf[i];
        CHConfigType tmp;
        return tmp;
    }
    bool isKey(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return true;
        return false;
    }
    bool verifyKeyValue(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key){
            int s=all_conf[i].value.size();
            for(int j=0;j<s;j++){
                string str=all_conf[i].value[j];
                unsigned int index=str.find("=");
                if(index==str.npos)return false;
                if(index==0)return false;//第一个字符不能是等于，后面可以有等于
            }
            return true;
        }
        return true;//空的，kv语法正确
    }
    string getExtra(string key){//获取额外数据
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return all_conf[i].extra;
        return "";
    }
    vector<string>* getValue(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return &all_conf[i].value;
        return NULL;
    }
    vector<string> readValue(string key){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key)return all_conf[i].value;
        vector<string> tmp;
        return tmp;
    }
    string getKeyValue(string key,string key2,string defaule_value=""){
        int k=all_conf.size();//开始处理
        for(int i=0;i<k;i++)if(all_conf[i].name==key){
            int s=all_conf[i].value.size();
            for(int j=0;j<s;j++){
                string str=all_conf[i].value[j];
                unsigned int index=str.find("=");
                if(index==str.npos)continue;
                if(index==0)continue;//第一个字符不能是等于，后面可以有等于
                if(str.substr(0,index)==key2)return str.substr(index+1);
            }
            return defaule_value;
        }
        return defaule_value;//空的也算kv吗
    }
    ch_conf(string path){
        vector<string> str=read_file(path);//读取文件
        int k=str.size();
        bool is_recv=false;
        //string text1;
        //string text2;
        CHConfigType conf;
        for(row=0;row<k;row++){//开始处理
            string line;
            line=str[row];
            int s=line.size();
            bool is_start=false;
            int lineType=0;
            for(column=0;column<s;column++){//开始处理
                char c=line[column];
                if(!is_start){
                    if(isspace(c))continue;//空白字符跳过
                    if(c==';')break;//这一行是注释，跳过这一整行
                    is_start=true;
                    if(c!='['){
                        if(!is_recv){
                            print_stack("格式错误！缺少左中括号。");
                            return;
                        }else{
                            lineType=2;//这一行的类型是整行行
                        }
                    }else{
                        lineType=1;//这一行的类型是中括号型
                        if(conf.name.size()>0)all_conf.push_back(conf);
                        CHConfigType conf_new;
                        conf=conf_new;
                        if(line.find("]")==line.npos){
                            print_stack("格式错误！未关闭的中括号！");
                            return;
                        }
                        conf.name=line.substr(line.find("[")+1,line.find("]")-1);
                        conf.extra=line.substr(line.find("]")+1);
                        if(conf.name.size()==0){
                            print_stack("格式错误！中括号内不能为空！");
                            return;
                        }
                        is_recv=true;
//                        cout<<"[]:::"<<text1<<endl<<text2<<endl;
                        break;
                    }
                }
                if(lineType==2){
                    conf.value.push_back(line);
                    break;
                }else{
                    print_stack("格式错误！未知字符。");
                    return;
                }
            }
        }
        if(conf.name.size()>0)all_conf.push_back(conf);
        //全部处理完成
    }
    vector<string> read_file(string path){
        vector<string> str;
        fstream fin;
        fin.open(path, ios::in);
        string tmp;
        while(getline(fin, tmp))str.push_back(tmp);
        fin.close();
        return str;
    }
private:
    vector<CHConfigType> all_conf;
    int row=0;
    int column=0;
    void print_stack(string text){
        //cout<<row<<":"<<column<<" "<<text<<endl;
        printf("%d:%d %s\n",row+1,column+1,text.c_str());
    }
};
