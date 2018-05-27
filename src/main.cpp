#include "aiuihelper.h"

#define Aiui AiuiHelper::ins()

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in srv_addr;
    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port =htons((u_short)1680);
    inet_pton(AF_INET, "192.168.43.16", &srv_addr.sin_addr);

    //多次尝试连接
    int count = 0;
    while (connect(socket_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        if (++count >= 5) {
            perror("connect");
            return 1;
        }
        sleep(1);
    }
    Aiui.resource_socket_fd = socket_fd;

    int c = 0;
    while (c != 27) {
        Aiui.start();
        c = getchar();
    }
    close(socket_fd);
    return 0;
}
