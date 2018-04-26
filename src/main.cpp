#include "aiuihelper.h"

#define Aiui AiuiHelper::ins()

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

const char* LScoketDomain = "/tmp/Aiui.domain";

int main()
{
    int socket_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }
    static struct sockaddr_un srv_addr;
    srv_addr.sun_family = AF_LOCAL;
    strcpy(srv_addr.sun_path, LScoketDomain);

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