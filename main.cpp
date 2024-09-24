#include <QApplication>
#include <stdio.h>
#include "pwb.h"
#include <QtMqtt/QMqttClient>
#ifdef Q_OS_WIN32
#include <stdio.h>
#include <winsock.h>
#include <array>

namespace util{
std::string escape(std::string& data){
    std::string rt;
    for (auto&& x : data){
        switch (x){
        case '\r': rt.push_back('\\'),rt.push_back('r'); break;
        case '\n': rt.push_back('\\'),rt.push_back('n'); break;
        case '\t': rt.push_back('\\'),rt.push_back('t'); break;
        case '\b': rt.push_back('\\'),rt.push_back('b'); break;
        default: rt.push_back(x);
        }
    }
    return rt;
}
}

int read_hv(std::string const& v,std::string const& cmd_str,std::string& rt){
  auto const& split_ip4 = [](std::string const& ip){
    std::vector<uint8_t> rt;
    for (auto iter = std::begin(ip); std::distance(iter,std::end(ip))>0;){
      auto iter_cur = iter;
      rt.emplace_back(std::stoi(iter_cur.base()));
      while(*iter_cur++!='.' && iter_cur!=std::end(ip));
      iter = iter_cur;
    }
    return rt;
  };
  auto ipv4 = split_ip4(v);
  if (ipv4.size()<4) return -1;
  char cmd[256];
  for (size_t i=0; i<cmd_str.length();++i) cmd[i] = cmd_str[i];
  cmd[cmd_str.length()]='\0';
  char ans[256] = "";
  WSADATA wsadata;
  WSAStartup(2, &wsadata);
  SOCKET sock = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
  SOCKADDR_IN sockaddr_in;
  std::memset(&sockaddr_in,0,sizeof(sockaddr_in));
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(10001);
  sockaddr_in.sin_addr.S_un.S_un_b.s_b1 = ipv4[0];
  sockaddr_in.sin_addr.S_un.S_un_b.s_b2 = ipv4[1];
  sockaddr_in.sin_addr.S_un.S_un_b.s_b3 = ipv4[2];
  sockaddr_in.sin_addr.S_un.S_un_b.s_b4 = ipv4[3];

  int connect_status = connect(sock,(SOCKADDR*)&sockaddr_in,sizeof(SOCKADDR_IN));
  if (connect_status!=0){ WSACleanup(); return -2; }
  send(sock,cmd,(int)strlen(cmd),0);
  for (;;){
    char buf[256] = "";
    int length = recv(sock, buf, sizeof(buf), 0);
    if (length <= 0) break;
    buf[length] = 0;
    strncat(ans, buf, sizeof(ans) - strlen(ans) - 1);
    char *crlf = strstr(ans, "\r\n");
    if (crlf != NULL){*crlf = 0; break;}
  }
  WSACleanup();
  rt = ans;
  return 0;
};
#endif
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    pwb window;
    window.setGeometry(
        0
        ,0
        ,window.size().width()
        ,window.size().height()
        );
    window.show();
    return a.exec();
}
