#include <stdio.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")
#pragma warning(disable: 4996)

using namespace std;

typedef struct {
    SOCKET client;
    char* id;
}account;

account clients[64];
int numclients = 0;



void removeClient(SOCKET client)
{
    // Tim vi tri cua client trong mang
    int i = 0;
    for (; i < numclients; i++)
        if (clients[i].client == client) break;
    if (i < numclients - 1)
        clients[i] = clients[numclients - 1];
    numclients--;
}


DWORD WINAPI ClientThread(LPVOID lpParam)
{
    SOCKET client = *(SOCKET*)lpParam;
    int ret;
    char buf[256];
    char acc[50]; // luu id nguoi dung
    char cmd[32], id[32], tmp[32], mess[150];
    bool protocheck = TRUE;

    // Xu ly dang nhap
    while (1)
    {
        ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0)
            return 0;
        buf[ret] = 0;

        ret = sscanf(buf, "%s %s %s", cmd, acc, tmp);

        if (ret != 2)
        {
            const  char* msg = "[CONNECT] ERROR error_message\n";
            send(client, msg, strlen(msg), 0);
        }
        else
        {
            if (strcmp(cmd, "[CONNECT]") != 0)
            {
                const  char* msg = "[CONNECT] ERROR error_message\n";
                send(client, msg, strlen(msg), 0);
            }
            else
            {
                // Luu id vao account
                clients[numclients].client = client;
                clients[numclients].id = acc;

                const  char* msg = "[CONNECT] OK\n";
                send(client, msg, strlen(msg), 0);

                // gui thong bao ket noi cua user den cac user khac
                char sbuf[256];
                sprintf(sbuf, "[USER_CONNECT] %s\n", acc);
                for (int i = 0; i < numclients; i++)
                    if (client != clients[i].client)
                        send(clients[i].client, sbuf, strlen(sbuf), 0);

                // Them client vao mang
                clients[numclients].client = client;
                numclients++;
                break;
            }
        }
    }

    // Cac chuc nang can thiet
    while (1)
    {
        ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0)
            continue;
        buf[ret] = 0;
        printf("Du lieu nhan duoc tu %d: %s", client, buf);

        ret = sscanf(buf, "%s %s %s", cmd, id, tmp);

        // Chuc nang dang xuat
        if (strcmp(cmd, "[DISCONNECT]") == 0)
        {
            protocheck = FALSE;
            if (ret != 1)
            {
                const  char* msg = "[DISCONNECT] ERROR error_message\n";
                send(client, msg, strlen(msg), 0);
            }
            else
            {
                const char* msg = "[DISCONNECT] OK\n";
                send(client, msg, strlen(msg), 0);

                char sbuf[256];
                sprintf(sbuf, "[USER_DISCONNECT] %s\n", acc);
                for (int i = 0; i < numclients; i++)
                    if (client != clients[i].client)
                        send(clients[i].client, sbuf, strlen(sbuf), 0);
                removeClient(client);
            }
        }

        // Xem danh sach 
        if (strcmp(cmd, "[LIST]") == 0)
        {
            protocheck = FALSE;
            if (ret != 1)
            {
                const  char* msg = "[LIST] ERROR error_message\n";
                send(client, msg, strlen(msg), 0);
            }
            else
            {
                char list[256];
                strcpy(list, "[LIST] OK ");
                for (int i = 0; i < numclients; i++) {
                    strcat(list, clients[i].id);
                    strcat(list, " ");
                }
                strcat(list, "\n");
                send(client, list, strlen(list), 0);
            }
        }

        // Chuc nang chuyen tin nhan
        if (strcmp(cmd, "[SEND]") == 0)
        {
            protocheck = FALSE;
            if (ret < 3)
            {
                const  char* msg = "[SEND] ERROR error_message\n";
                send(client, msg, strlen(msg), 0);
            }
            else
            {
                const char* msg = "[SEND] OK\n";
                char sbuf[256];
                // kiem tra xem gui duoc khong
                bool check = FALSE;

                // Tach message tu buf
                strcpy(mess, &buf[strlen(cmd) + strlen(id) + 2]);

                // Gui tin nhan cho nhom
                if (strcmp(id, "ALL") == 0)
                {
                    sprintf(sbuf, "[MESSAGE_ALL] %s %s\n", acc, mess);
                    for (int i = 0; i < numclients; i++)
                        if (client != clients[i].client)
                        {
                            check = TRUE;
                            send(clients[i].client, sbuf, strlen(sbuf), 0);
                        }
                }
                else // Gui tin nhan cho ca nhan
                {
                    sprintf(sbuf, "[MESSAGE] %s %s\n", acc, mess);
                    for (int i = 0; i < numclients; i++)
                    {
                        if (strcmp(id, clients[i].id) == 0)
                        {
                            check = TRUE;
                            send(clients[i].client, sbuf, strlen(sbuf), 0);
                        }
                    }
                }
                // Thong bao gui thanh cong hay that bai
                if (check)
                {
                    send(client, msg, strlen(msg), 0);
                }
                else
                {
                    const char* msg = "[SEND] ERROR error_message\n";
                    send(client, msg, strlen(msg), 0);
                }
            }
        }
        // Kiem tra xem co sai giao thuc khong
        if (protocheck)
        {
            const char* msg = "[ERROR] error_message\n";
            send(client, msg, strlen(msg), 0);
        }
    }
    closesocket(client);
}

int main()
{
    // Khoi tao thu vien
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Tao Socket nhan cua server
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Khai bao dia chi server
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    // Gan cau truc dia chi voi socket
    bind(listener, (SOCKADDR*)&addr, sizeof(addr));

    // Chuyen sang trang thai ket noi
    listen(listener, 5);
    while (1)
    {
        SOCKET client = accept(listener, NULL, NULL);
        printf("Client moi ket noi: %d\n", client);
        CreateThread(0, 0, ClientThread, &client, 0, 0);
    }
}