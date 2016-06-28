#include <boost/thread.hpp>
#include <vector>
#include <WinSock2.h>
#include <Mswsock.h>

#include "../../vocal/vocal.h"

#include <packon.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#include <packoff.h>

HANDLE iocp;
int client_index;

struct overlappedex : public OVERLAPPED
{
	int event_type;//0 accept, 1 recv, 2 send
	SOCKET s; // accept || recv || send SOCKET
	LPWSABUF buf;//send || recv buf
};

int main() 
{
	WSAData data;
	WSAStartup(2, &data);

	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in laddr;
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(0);
	laddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (bind(s, (sockaddr*)&laddr, sizeof(laddr)) != 0) {
		printf("bind error=%d\n", WSAGetLastError());
		return -1;
	}

	sockaddr_in raddr;
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(7777);
	raddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	
	if (connect(s, (sockaddr*)&raddr, sizeof(sockaddr_in)) != 0) {
		printf("connect error=%d\n", WSAGetLastError());
		return -1;
	}

	if (CreateIoCompletionPort((HANDLE)s, iocp, 0, 1) != iocp) {
		printf("CreateIoCompletionPort error=%d\n", WSAGetLastError());
		return -1;
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
		ov->s = s;
		ov->event_type = 1;
		OVERLAPPED * ovp = static_cast<OVERLAPPED *>(ov);
		WSARecv(s, recvbuf, 1, &recv, &flag1, ovp, 0);
	}

	paInit _paInit;
	sound _sound;

	auto inputd = devices::getInputDevices();
	auto outputd = devices::getOutputDevices();
	 
	if (!inputd.empty())
	{
		_sound.setInputDevice(inputd.begin()->first);
	}
	if (!outputd.empty())
	{
		_sound.setOutputDevice(outputd.begin()->first);
	}

	_sound.sigCapture.connect([s](short channelcount, char * buff, int len){
		LPWSABUF sendbuf = new WSABUF;
		sendbuf->buf = new char[len + 14];
		sendbuf->len = len + 14;
		*(int*)sendbuf->buf = 2;
		*(int*)(sendbuf->buf + 4) = client_index;
		*(short*)(sendbuf->buf + 8) = channelcount;
		*(int*)(sendbuf->buf + 10) = len;
		memcpy(sendbuf->buf + 14, buff, len);
		overlappedex * ov1 = new overlappedex;
		memset(ov1, 0, sizeof(overlappedex));
		ov1->s = s;
		ov1->buf = sendbuf;
		ov1->event_type = 2;
		OVERLAPPED * ovp1 = static_cast<OVERLAPPED *>(ov1);
		DWORD send = 0, flag = 0;
		WSASend(s, sendbuf, 1, &send, flag, ovp1, 0);
	});

	while (1) {
		DWORD bytes = 0;
		ULONG_PTR ptr = 0;
		LPOVERLAPPED ovp = 0;

		if (GetQueuedCompletionStatus(iocp, &bytes, &ptr, &ovp, -1)) {
			overlappedex * ovlp = static_cast<overlappedex *>(ovp);
			
			if (ovlp->event_type == 1)
			{
				char * buff = ovlp->buf->buf;
				auto event_type = *(int*)buff;
				if (event_type == 0) {
					client_index = *(int*)(buff + 4);
					
					_sound.start();
				}
				else if (event_type == 1) {
					int new_client = *(int*)(buff + 4);
					create_client(new_client);
				}
				else if (event_type == 2) {
					int _client_index = *(int*)(buff + 4);
					auto _client = get_client(_client_index);
					short channelcount = *(short*)(buff + 8);
					int len = *(int*)(buff + 10);
					if (_client) {
						_client->write_buff(buff + 14, len, channelcount);
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
					WSARecv(s, recvbuf, 1, &recv, &flag1, ovp, 0);
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

	return 1;
}