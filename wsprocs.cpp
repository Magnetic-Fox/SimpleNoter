#include "wsprocs.hpp"

int wsInit(void) {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(1,1), &wsaData);
}

char* getHostIP(char* hostname/*, DWORD* dwError*/) {
    struct hostent *remoteHost;
    struct in_addr addr;

    remoteHost=gethostbyname(hostname);

    if(remoteHost==NULL) {
        // *dwError=WSAGetLastError();
        return NULL;
        /*
        if(dwError!=0) {
            if(dwError == WSAHOST_NOT_FOUND) {
                std::cout << "Host not found." << std::endl;
            }
            else if(dwError == WSANO_DATA) {
                std::cout << "No data record found." << std::endl;
            }
            else {
                std::cout << "Function failed with error: " << dwError << std::endl;
            }
        }
        */
    }
    else {
        // *dwError=0;
        if(remoteHost->h_addrtype == AF_INET) {
            /*
            int i=0;
            while(remoteHost->h_addr_list[i] != 0) {
                addr.s_addr = *(u_long *) remoteHost->h_addr_list[i++];
                std::cout << "IP #" << i << ": " << inet_ntoa(addr) << std::endl;
            }
            */

            if(remoteHost->h_addr_list[0] != 0) {
                addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
                return inet_ntoa(addr);
            }
            else {
                return NULL;
            }
        }
        else {
            return NULL;
        }
    }
}
