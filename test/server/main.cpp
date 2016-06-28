#include <boost/thread.hpp>
#include <vector>
#include <WinSock2.h>
#include <Mswsock.h>

#include <packon.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#include <packoff.h>

SOCKET s_listen;
char * outbuf;
HANDLE iocp;
std::vector<SOCKET> client_set;

struct overlappedex : public OVERLAPPED
{
	int event_type;//0 accept, 1 recv, 2 send
	SOCKET s; // accept || recv || send SOCKET
	LPWSABUF buf;//send || recv buf
};

void do_net() {
	while (1)
	{
		DWORD bytes = 0;
		ULONG_PTR ptr = 0;
		LPOVERLAPPED ovp = 0;

		if (GetQueuedCompletionStatus(iocp, &bytes, &ptr, &ovp, -1)) {
			overlappedex * ovlp = static_cast<overlappedex *>(ovp);

			if (ovlp->event_type == 0) {
				// bind iocp
				if (CreateIoCompletionPort((HANDLE)ovlp->s, iocp, 0, 1) != iocp) {
					printf("CreateIoCompletionPort error=%d\n", WSAGetLastError());
				}

				// send client index
				{
					LPWSABUF sendbuf = new WSABUF;
					sendbuf->buf = new char[8];
					sendbuf->len = 8;
					*(int*)sendbuf->buf = 0;
					*(int*)(sendbuf->buf + 4) = client_set.size();
					overlappedex * ov1 = new overlappedex;
					memset(ov1, 0, sizeof(overlappedex));
					ov1->s = ovlp->s;
					ov1->buf = sendbuf;
					ov1->event_type = 2;
					OVERLAPPED * ovp1 = static_cast<OVERLAPPED *>(ov1);
					DWORD send = 0, flag = 0;
					WSASend(ovlp->s, sendbuf, 1, &send, flag, ovp1, 0);

					for (int i = 0; i < client_set.size(); i++)
					{
						LPWSABUF sendbuf = new WSABUF;
						sendbuf->buf = new char[8];
						sendbuf->len = 8;
						*(int*)sendbuf->buf = 1;
						*(int*)(sendbuf->buf + 4) = i;
						overlappedex * ov1 = new overlappedex;
						memset(ov1, 0, sizeof(overlappedex));
						ov1->s = ovlp->s;
						ov1->buf = sendbuf;
						ov1->event_type = 2;
						OVERLAPPED * ovp1 = static_cast<OVERLAPPED *>(ov1);
						DWORD send = 0, flag = 0;
						WSASend(ovlp->s, sendbuf, 1, &send, flag, ovp1, 0);
					}

					for (auto s : client_set)
					{
						LPWSABUF sendbuf = new WSABUF;
						sendbuf->buf = new char[8];
						sendbuf->len = 8;
						*(int*)sendbuf->buf = 1;
						*(int*)(sendbuf->buf + 4) = client_set.size();
						overlappedex * ov1 = new overlappedex;
						memset(ov1, 0, sizeof(overlappedex));
						ov1->s = s;
						ov1->buf = sendbuf;
						ov1->event_type = 2;
						OVERLAPPED * ovp1 = static_cast<OVERLAPPED *>(ov1);
						DWORD send = 0, flag = 0;
						WSASend(s, sendbuf, 1, &send, flag, ovp1, 0);
					}
				}

				// recv client data
				{
					LPWSABUF recvbuf = new WSABUF;
					recvbuf->buf = new char[8 * 1024 * 1024];
					recvbuf->len = 8 * 1024 * 1024;
					DWORD recv = 0, flag1 = 0;
					overlappedex * ov = new overlappedex;
					memset(ov, 0, sizeof(overlappedex));
					ov->buf = recvbuf;
					ov->s = ovlp->s;
					ov->event_type = 1;
					OVERLAPPED * ovp = static_cast<OVERLAPPED *>(ov);
					WSARecv(ovlp->s, recvbuf, 1, &recv, &flag1, ovp, 0);
				}

				// re accept
				{
					SOCKET s_accept = socket(AF_INET, SOCK_STREAM, 0);
					overlappedex * ov = new overlappedex;
					memset(ov, 0, sizeof(overlappedex));
					ov->s = s_accept;
					ov->event_type = 0;
					OVERLAPPED * ovp = static_cast<OVERLAPPED *>(ov);
					AcceptEx(s_listen, s_accept, outbuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, ovp);
				}

				// 
				client_set.push_back(ovlp->s);
			}
			else if (ovlp->event_type == 1)
			{
				for (auto s : client_set)
				{
					if (s != ovlp->s)
					{
						LPWSABUF sendbuf = new WSABUF;
						sendbuf->buf = new char[bytes];
						sendbuf->len = bytes;
						memcpy(sendbuf->buf, ovlp->buf->buf, bytes);
						overlappedex * ov1 = new overlappedex;
						memset(ov1, 0, sizeof(overlappedex));
						ov1->s = ovlp->s;
						ov1->buf = sendbuf;
						ov1->event_type = 2;
						OVERLAPPED * ovp1 = static_cast<OVERLAPPED *>(ov1);
						DWORD send = 0, flag = 0;
						WSASend(s, sendbuf, 1, &send, flag, ovp1, 0);
					}
				}

				// recv client data
				{
					LPWSABUF recvbuf = new WSABUF;
					recvbuf->buf = new char[8 * 1024 * 1024];
					recvbuf->len = 8 * 1024 * 1024;
					DWORD recv = 0, flag1 = 0;
					overlappedex * ov = new overlappedex;
					memset(ov, 0, sizeof(overlappedex));
					ov->buf = recvbuf;
					ov->s = ovlp->s;
					ov->event_type = 1;
					OVERLAPPED * ovp = static_cast<OVERLAPPED *>(ov);
					WSARecv(ovlp->s, recvbuf, 1, &recv, &flag1, ovp, 0);
				}

				delete[] ovlp->buf->buf;
				delete ovlp->buf;
			}
			else if (ovlp->event_type == 2)
			{
				delete[] ovlp->buf->buf;
				delete ovlp->buf;
			}

			delete ovlp;
		}
	}
}

int main(){
	WSAData data;
	WSAStartup(2, &data);

	boost::thread_group _group;

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
	
	s_listen = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in laddr;
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(7777);
	laddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (bind(s_listen, (sockaddr*)&laddr, sizeof(laddr)) != 0){
		printf("bind error=%d\n", WSAGetLastError());
		return -1;
	}

	if (listen(s_listen, 10) != 0) {
		printf("listen error=%d\n", WSAGetLastError());
		return -1;
	}

	if (CreateIoCompletionPort((HANDLE)s_listen, iocp, 0, 1) != iocp) {
		printf("CreateIoCompletionPort error=%d\n", WSAGetLastError());
		return -1;
	}

	SOCKET s_accept = socket(AF_INET, SOCK_STREAM, 0);

	outbuf = new char[1024];

	overlappedex * ov = new overlappedex;
	memset(ov, 0, sizeof(overlappedex));
	ov->s = s_accept;
	ov->event_type = 0;
	OVERLAPPED * ovp = static_cast<OVERLAPPED *>(ov);
	AcceptEx(s_listen, s_accept, outbuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, ovp);

	_group.create_thread(do_net);

	_group.join_all();

	WSACleanup();

	return 0;
}
