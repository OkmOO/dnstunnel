#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "app.h"
#include "../include/util.h"
#include "../include/udp.h"
#include "../include/cmd.h"
/*
* client端定时询问server，是否有命令需要执行
*/
extern char g_baseDomain[255];

static void daemonlize()
{
    if (fork() == 0)
    {
        if (fork() == 0)
        {
            close(0);
            close(1);
            close(2);
            return;
        }
    }
    exit(0);
}

static void get_sys_nameserver(char * server, int len)
{
    FILE * fp = fopen("/etc/resolv.conf", "r");
    if (fp)
    {
        do
        {
            char line[255] = {0};
            (void)fgets(line, sizeof(line), fp);
            if (line[0] == '#')
            {
                continue;
            }

            if (strncmp("nameserver ", line, 11) == 0)
            {
                char * p = line + 11;
                char * out = (char *)server;
                while(*p != '\n')
                    *(out++) = *(p++);
                *out = 0;
                break;
            }
        }while(!feof(fp));
        fclose(fp);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("%s <baseDomain>\n", argv[0]);
        return 1;
    }
    
    //daemonlize();
    strcpy_s(g_baseDomain, 255, argv[1]);

    client_app_init();

    char dns_server_ip[255];
    get_sys_nameserver(dns_server_ip, sizeof(dns_server_ip));

    int fd = udp_connect(dns_server_ip, 53);
    if(fd == -1)
    {
        perror("udp connect fail");
        return 1;
    }

    char req[65536];
    char rsp[1024*1024];
    while(1){
        int len = client_recv(fd, req, sizeof(req));

        struct CmdReq * cmd = (struct CmdReq *)req;
        len = handleCmd(cmd, rsp, sizeof(rsp));
        if (len > 0)
        {
            len = client_send(fd, rsp, len);
        }

        delay(1, 0);
    }
        
    return 0;
}