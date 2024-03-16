#include "requests.hpp"

int getResponseCode(HEADERS &headers) {
    return atoi(headers["HTTP_Response_Code"].c_str());
}

std::string prepareRequest(char* requestType, char* place, char* userAgent, char* ipAddress, unsigned int port, char* contentType, char* content) {
    std::string temp="";
    temp=temp+requestType+" ";
    temp=temp+"/"+place+" HTTP/1.0\r\n";
    temp=temp+"User-Agent: "+userAgent+"\r\n";
    temp=temp+"Host: "+ipAddress;
    if(port!=80) {
        char portS[6];
        ltoa(port,portS,10);
        temp=temp+":"+portS;
    }
    temp=temp+"\r\n";
    if(strlen(contentType)>0) {
        temp=temp+"Content-type: "+contentType+"\r\n";
    }
    long int contentLength=strlen(content);
    if(contentLength>0) {
        char cLen[16];
        ltoa(contentLength,cLen,10);
        temp=temp+"Content-Length: "+cLen+"\r\n\r\n";
        temp=temp+content;
    }
    else {
        temp=temp+"\r\n\r\n";
    }
    return temp;
}

void interpretHeaderLine(HEADERS& headers, std::string oneline, bool first) {
    std::string temp="", temp2="";
    if(first) {
        first=false;
        int level=0;
        for(unsigned int x=0; x<oneline.length(); ++x) {
            if(level<2) {
                if(oneline[x]!=0x20) {
                    temp=temp+oneline[x];
                }
                else {
                    if(level==0) {
                        headers["HTTP"]=temp;
                    }
                    else {
                        headers["HTTP_Response_Code"]=temp;
                    }
                    ++level;
                    temp="";
                }
            }
            else {
                temp=temp+oneline[x];
            }
        }
        headers["HTTP_Response_Text"]=temp;
    }
    else {
        bool name=true, escape=true;
        for(unsigned int x=0; x<oneline.length(); ++x) {
            if(name) {
                if(oneline[x]==':') {
                    name=false;
                }
                else {
                    temp2=temp2+oneline[x];
                }
            }
            else {
                if(oneline[x]!=0x20) {
                    escape=false;
                }
                if(!escape) {
                    temp=temp+oneline[x];
                }
            }
        }
        headers[temp2]=temp;
    }
    return;
}

std::string URLencode(char* input) {
    std::string temp="";
    for(unsigned int x=0; x<strlen(input); ++x) {
        if((input[x]>='0' && input[x]<='9') || (input[x]>='A' && input[x]<='Z') || (input[x]>='a' && input[x]<='z')) {
            temp=temp+input[x];
        }
        else {
            char conv[3];
            itoa(input[x],conv,16);
            for(unsigned int y=0; y<strlen(conv); ++y) {
                conv[y]=toupper(conv[y]);
            }
            if(input[x]<16) {
                temp=temp+"%0"+conv;
            }
            else {
                temp=temp+'%'+conv;
            }
        }
    }
    return temp;
}

std::string prepareContent(char* action, char* username, char* password, char* subject, char* entry, unsigned long int noteID, char* newPassword, bool requestCompression) {
    std::string temp="action=";
    temp=temp+URLencode(action);
    temp=temp+"&username=";
    temp=temp+URLencode(username);
    temp=temp+"&password=";
    temp=temp+URLencode(password);
    temp=temp+"&subject=";
    temp=temp+URLencode(subject);
    temp=temp+"&entry=";
    temp=temp+URLencode(entry);
    temp=temp+"&noteID=";
    if(noteID>0) {
        char conv[16];
        ltoa(noteID,conv,10);
        temp=temp+conv;
    }
    temp=temp+"&newPassword=";
    temp=temp+URLencode(newPassword);
    temp=temp+"&compress=";
    if(requestCompression) {
        temp=temp+"yes";
    }
    else {
        temp=temp+"no";
    }
    return temp;
}

unsigned int makeRequest(char* ipAddress, unsigned int port, char* method, char* place, char* userAgent, char* contentType, char* content, HEADERS &headers, char* output) {
    unsigned int pos=0;
    SOCKET mainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if(mainSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    else {
        sockaddr_in service;
        memset(&service, 0, sizeof(service));
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = inet_addr(ipAddress);
        service.sin_port = htons(port);

        if(connect(mainSocket, (SOCKADDR *) &service, sizeof(service)) == SOCKET_ERROR) {
            return 0;
        }
        else {
            int bytesSent;
            int bytesRecv;
            char recvbuf[2048];

            std::string req=prepareRequest(method,place,userAgent,ipAddress,port,contentType,content);

            bytesSent = send(mainSocket, req.c_str(), strlen(req.c_str()), 0);

            if(bytesSent<0) {
                return 0;
            }
            else {
                if(bytesSent!=strlen(req.c_str())) {
                    return 0;
                }
            }

            std::string oneline="";
            char lastCharacter=0x00;
            bool first=true, readingHeaders=true;

            while(true) {
                bytesRecv = recv(mainSocket, recvbuf, 2048, 0);

                if(bytesRecv == 0 || bytesRecv == WSAECONNRESET) {
                    break;
                }

                for(unsigned int x=0; x<bytesRecv; ++x) {
                    if(readingHeaders) {
                        if(recvbuf[x]>=0x20) {
                            lastCharacter=recvbuf[x];
                            oneline=oneline+lastCharacter;
                        }
                        else {
                            if(lastCharacter==0x0D && recvbuf[x]==0x0A) {
                                if(oneline.length()==0) {
                                    readingHeaders=false;
                                }
                                else {
                                    interpretHeaderLine(headers,oneline,first);
                                    oneline="";
                                }
                                if(first) {
                                    first=false;
                                }
                            }
                            lastCharacter=recvbuf[x];
                        }
                    }
                    else {
                        output[pos++]=recvbuf[x];
                        if(pos==0) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return pos;
}
