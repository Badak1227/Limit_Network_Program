/*
* 이승진_20203082
* 이름 : LNP (Limit_Network_Program)
* 목적:
* TCP와 UDP를 이용한 서버와 클라이언트를 만들고 클라이언트가 서버로 메시지를 보내면 서버가 메시지를 받고 삭제하며,
* 클라이언트는 500byte/s, 1000byte/s, 2000byte/s의 속도로 데이터를 보낼 수 있다.
*
* 운영체제 : window11 와 window10
* 개발 IDE : visual studio
* 컴파일러 : visual studio compiler
*/
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ESC 27
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

char msg[101] = { 0 };

int get_ip(SOCKADDR_IN* addr, int type) {
	struct addrinfo* result;
	struct addrinfo hints;
	struct sockaddr_in* local_addr;

	int iresult = 0;
	char myaddr[256];

	if (gethostname(myaddr, sizeof(myaddr)) == -1) return -1;

	printf(" hostname : %s\n", myaddr);

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // IPv4 주소를 얻기 위해 AF_INET 사용
	hints.ai_socktype = type;
	hints.ai_protocol = 0;

	iresult = getaddrinfo(myaddr, NULL, &hints, &result);
	if (iresult != 0) {
		printf("%d\n", iresult);
		return -1;
	}
	local_addr = (struct sockaddr_in*)result->ai_addr;

	printf(" ip : %s\n", inet_ntoa(local_addr->sin_addr));
	addr->sin_addr = local_addr->sin_addr;//htonl(INADDR_ANY);

	return 0;
}

int tcp_server() {
	WSADATA wsa_init;
	SOCKET server, client;
	SOCKADDR_IN server_addr, client_addr;
	u_long socket_mode = 1; //비동기 소켓 모드
	SYSTEMTIME sent_time, received_time;
	FILE* received_csv = NULL;

	int client_addr_size, len = 0, port = 2000;
	char type = 0, received_msg[100] = { 0 };

	if (WSAStartup(MAKEWORD(2, 2), &wsa_init)) {
		printf(" WSA init failed\n");
		return 0;
	}

	system("cls");
	printf(" Open TCP socket...  ");

	server = socket(AF_INET, SOCK_STREAM, 0);
	ioctlsocket(server, FIONBIO, &socket_mode);

	//소켓 생성 오류 확인
	if (server == INVALID_SOCKET) {
		printf("failed.\n");
		WSACleanup();
		return 0;
	}
	printf("success.\n");

	//bind() 및 오류확인
	server_addr.sin_family = AF_INET;
	//현재 호스트의 IPv4 주소를 얻어온다.
	if (get_ip(&server_addr, SOCK_STREAM) == -1) {
		printf(" Get ip address failed\n");

		closesocket(server);
		WSACleanup();
		return 0;
	}

	//사용 가능한 포트번호 부여
	for (port = 2000; port < 65536; port++) {
		server_addr.sin_port = htons(port);
		if (bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) break;
	}

	if (port == 65536) {
		printf(" Socket bind failed.\n");

		closesocket(server);
		WSACleanup();
		return 0;
	}
	else printf(" port : %d\n", port);

	//listen() 및 오류 확인 - TCP서버는 listen()을 통해 연결 대기 필요
	if (listen(server, 20) == -1) {
		printf(" Socket listen failed.\n");

		closesocket(server);
		WSACleanup();
		return 0;
	}

	printf(" Back | esc\n\n");

	received_csv = fopen("received.csv", "w");//csv파일로 수신받은 패킷의 송신 시간과 수신시간을 기록
	client_addr_size = sizeof(client_addr);

	while (1) {
		//esc를 누를 경우 accept 대기 상태에서 벗어남
		if (_kbhit() && _getch() == ESC) {
			closesocket(client);
			break;
		}

		//accept() - TCP서버는 accept()를 이용	
		client = accept(server, (struct sockaddr*)&client_addr, &client_addr_size);

		if (client != -1) printf(" Client %s connected - Disconnect client | esc\n", inet_ntoa(client_addr.sin_addr));

		//클라이언트 연결이 끊어질 때 까지 메시지를 읽음
		while (client != -1) {
			//esc를 누를 경우 메시지 수신 대기 상태에서 벗어남
			if (_kbhit() && _getch() == ESC) {
				printf(" Disconnect client\n");
				break;
			}

			len = recv(client, received_msg, 100, 0);

			//client측에서 연결 종료
			if (len == 0) {
				printf(" Client disconnected\n\n");
				break;
			}
			//msg 수신시 동작
			else if (len > 0) {
				GetSystemTime(&received_time);

				printf(" sent time : %s\n", received_msg);
				printf(" receive time: %02d:%02d:%02d.%03d\n\n", received_time.wHour, received_time.wMinute, received_time.wSecond, received_time.wMilliseconds);
				fprintf(received_csv, "%s,%02d:%02d:%02d.%03d\n", received_msg, received_time.wHour, received_time.wMinute, received_time.wSecond, received_time.wMilliseconds);
			}
		}

		closesocket(client);
		client = -1;
	}

	fclose(received_csv);
	closesocket(server);
	WSACleanup();
	system("cls");
	return 1;
}

int udp_server() {
	WSADATA wsa_init;
	SOCKET server, client = -1;
	SOCKADDR_IN server_addr, client_addr;
	u_long socket_mode = 1; //비동기 소켓 모드
	SYSTEMTIME sent_time, received_time;
	FILE* received_csv = NULL;

	int client_addr_size, transmission_rate = 0, len = 0, port = 2000;
	char type = 0, received_msg[100] = { 0 };

	if (WSAStartup(MAKEWORD(2, 2), &wsa_init)) {
		printf(" WSA init failed\n");
		return 0;
	}

	system("cls");
	printf(" Open UDP socket...  ");

	server = socket(AF_INET, SOCK_DGRAM, 0);
	ioctlsocket(server, FIONBIO, &socket_mode);

	//소켓 생성 오류 확인
	if (server == -1) {
		printf("failed.\n");
		WSACleanup();
		return 0;
	}
	printf("success.\n");

	//bind() 및 오류확인
	server_addr.sin_family = AF_INET;
	//현재 호스트의 IPv4 주소를 얻어온다.
	if (get_ip(&server_addr, SOCK_DGRAM) == -1) {
		printf(" Get ip address failed\n");

		closesocket(server);
		WSACleanup();
		return 0;
	}

	//사용가능한 포트번호 부여
	for (port = 2000; port < 65536; port++) {
		server_addr.sin_port = htons(port);
		if (bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) break;
	}

	if (port == 65536) {
		printf(" Socket bind failed.\n");

		closesocket(server);
		WSACleanup();
		return 0;
	}
	else printf(" port : %d\n", port);
	printf(" Back | esc\n\n");

	received_csv = fopen("received.csv", "w");//csv파일로 수신받은 패킷의 송신 시간과 수신시간을 기록
	client_addr_size = sizeof(client_addr);

	while (1) {
		//esc를 누를 경우 메시지 수신 대기 상태에서 벗어남
		if (_kbhit() && _getch() == ESC) {
			closesocket(client);
			break;
		}

		//recvfrom을 통해 받은 메시지 바로 읽어들이기
		len = recvfrom(server, received_msg, 100, 0, (struct sockaddr*)&client_addr, &client_addr_size);
		if (len != -1) {
			GetSystemTime(&received_time);

			printf(" sent time : %s\n", received_msg);
			printf(" receive time: %02d:%02d:%02d.%03d from %s\n", received_time.wHour, received_time.wMinute, received_time.wSecond, received_time.wMilliseconds, inet_ntoa(client_addr.sin_addr));
			fprintf(received_csv, "%s,%02d:%02d:%02d.%03d\n", received_msg, received_time.wHour, received_time.wMinute, received_time.wSecond, received_time.wMilliseconds);

			closesocket(client);
			client = -1;
		}
	}

	fclose(received_csv);
	closesocket(server);
	WSACleanup();
	system("cls");
	return 1;
}

int limit_tcp_send(SOCKET sock, int max_rate) {
	SYSTEMTIME time;
	ULONGLONG prev_tick = GetTickCount64();
	ULONGLONG current_tick = 0;
	FILE* sent_csv = NULL;

	int sent_bytes = 0, sent;

	sent_csv = fopen("sent.csv", "w");//패킷 손실율을 측정하기 위해 패킷들의 송신시간을 기록
	do {
		//esc를 누를 경우 반복 송신 상태에서 벗어남 (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		if (_kbhit() && _getch() == ESC) break;

		current_tick = GetTickCount64();

		if (current_tick - prev_tick >= 1000) { //1초 경과
			prev_tick = current_tick;
			sent_bytes = 0; //전송량 초기화
		}

		//전송량 체크 max_rate보다 보낸 byte 수가 많으면 데이터를 송신하지 않는다.
		if (sent_bytes >= max_rate) {
			Sleep(1); //1밀리초 대기
			continue;
		}

		GetSystemTime(&time);
		sprintf(msg, "%02hu:%02hu:%02hu.%03hu", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

		sent = send(sock, msg, 100, 0);
		if (sent == -1) {
			printf(" Send message failed\n");
			return -1;
		}
		else {
			sent_bytes += sent;

			printf("Sent time: %02d:%02d:%02d.%03d\n\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
			fprintf(sent_csv, "%02d:%02d:%02d.%03d\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		}
	} while (1);

	fclose(sent_csv);
	return 0;
}

int tcp_client() {
	WSADATA wsa_init;
	SOCKET client;
	SOCKADDR_IN server_addr;
	u_long socket_mode = 1; //비동기 소켓 모드

	int server_addr_size, loop = 0, len = 0, server_port = 1025;
	char type = 0, transmission_rate = 0, server_ip[40] = { 0 };
	system("cls");

	if (WSAStartup(MAKEWORD(2, 2), &wsa_init) != 0) {
		printf(" WSA init failed.\n");
		return 0;
	}

	printf(" Open TCP socket...  ");

	client = socket(AF_INET, SOCK_STREAM, 0);
	ioctlsocket(client, FIONBIO, &socket_mode);

	//소켓 생성 오류 확인
	if (client == -1) {
		printf("failed.\n");

		closesocket(client);
		WSACleanup();
		return 0;
	}
	printf("success.\n");

	//서버 ip와 port 번호 입력
	printf(" Enter server ip : ");
	scanf("%s", server_ip);
	printf(" Enter server port : ");
	scanf("%d", &server_port);
	while (getchar() != '\n');

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons((u_short)server_port);

	server_addr_size = sizeof(server_addr);

	//서버와 connect 생성
	if (connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);

	printf(" Select transmission rate - 500byte/s | 1     1000byte/s | 2     2000byte/s | 3\n");
	do {
		transmission_rate = _getch();

		if ('1' <= transmission_rate && transmission_rate <= '3') {
			printf("\r                     \r %c\n", transmission_rate);
			break;
		}
		//esc가 들어올 경우 뒤로 간다.
		else if (transmission_rate == ESC) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 1;
		}
		else {
			printf("\r WRONG INPUT OCCURED");
		}
	} while (1);

	//500byte/s
	if (transmission_rate == '1') {
		if (limit_tcp_send(client, 500) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}
	//1000byte/s
	else if (transmission_rate == '2') {
		if (limit_tcp_send(client, 1000) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}
	//2000byte/s
	else if (transmission_rate == '3') {
		if (limit_tcp_send(client, 2000) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}

	closesocket(client);
	WSACleanup();
	system("cls");
	return 1;
}

int limit_udp_send(SOCKET sock, struct sockaddr* to, int tolen, int max_rate) {
	SYSTEMTIME time;
	ULONGLONG prev_tick = GetTickCount64();
	ULONGLONG current_tick = 0;
	FILE* sent_csv;

	int sent_bytes = 0, sent;

	sent_csv = fopen("sent.csv", "w");//패킷 손실율을 측정하기 위해 패킷들의 송신시간을 기록

	do {
		//esc를 누를 경우 반복 송신 상태에서 벗어남 (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		if (_kbhit() && _getch() == ESC) break;

		current_tick = GetTickCount64();

		if (current_tick - prev_tick >= 1000) { //1초 경과
			prev_tick = current_tick;
			sent_bytes = 0; //전송량 초기화
		}

		//전송량 체크 max_rate보다 보낸 byte 수가 많으면 데이터를 송신하지 않는다.
		if (sent_bytes >= max_rate) {
			Sleep(1); //1밀리초 대기
			continue;
		}

		GetSystemTime(&time);
		sprintf(msg, "%02hu:%02hu:%02hu.%03hu", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		sent = sendto(sock, msg, 100, 0, to, tolen);
		if (sent == -1) {
			printf(" Send message failed\n");
			return -1;
		}
		else {
			sent_bytes += sent;

			printf("Sent time: %02d:%02d:%02d.%03d\n\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
			fprintf(sent_csv, "%02d:%02d:%02d.%03d\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		}
	} while (1);

	fclose(sent_csv);
	return 0;
}

int udp_client() {
	WSADATA wsa_init;
	SOCKET client;
	SOCKADDR_IN server_addr;
	u_long socket_mode = 1; //비동기 소켓 모드

	int server_addr_size, len = 0, loop = 0, server_port = 1025;
	char type = 0, transmission_rate = 0, server_ip[40] = { 0 };
	system("cls");

	if (WSAStartup(MAKEWORD(2, 2), &wsa_init) != 0) {
		printf(" WSA init failed.\n");
		return 0;
	}

	printf(" Open UDP socket...  ");

	client = socket(AF_INET, SOCK_DGRAM, 0);
	ioctlsocket(client, FIONBIO, &socket_mode);

	//소켓 생성 오류 확인
	if (client == -1) {
		printf("failed.\n");
		WSACleanup();
		return 0;
	}
	printf("success.\n");

	//서버 ip와 port 번호 입력
	printf("Enter server ip : ");
	scanf("%s", server_ip);
	printf("Enter server port : ");
	scanf("%d", &server_port);
	while (getchar() != '\n');

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	server_addr.sin_port = htons((u_short)server_port);

	server_addr_size = sizeof(server_addr);

	printf(" Select transmission rate - 500byte/s | 1     1000byte/s | 2     2000byte/s | 3\n");
	do {
		transmission_rate = _getch();

		if ('1' <= transmission_rate && transmission_rate <= '3') {
			printf("\r                     \r %c\n", transmission_rate);
			break;
		}
		//esc가 들어올 경우 뒤로 간다.
		else if (transmission_rate == ESC) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 1;
		}
		else {
			printf("\r WRONG INPUT OCCURED");
		}
	} while (1);

	if (transmission_rate == '1') {
		if (limit_udp_send(client, (struct sockaddr*)&server_addr, server_addr_size, 500) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}
	else if (transmission_rate == '2') {
		if (limit_udp_send(client, (struct sockaddr*)&server_addr, server_addr_size, 1000) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}
	else if (transmission_rate == '3') {
		if (limit_udp_send(client, (struct sockaddr*)&server_addr, server_addr_size, 2000) == -1) {
			closesocket(client);
			WSACleanup();
			system("cls");
			return 0;
		}
	}

	closesocket(client);
	WSACleanup();
	system("cls");
	return 1;
}

int main() {
	int loop = 1;
	char sc_select = 0, type = 0;

	system("cls");

	//본인이 서버인지 클라이언트인지 선택
	do {
		printf(" Are you a server or a client?\n\n server | S or s\n client | C or c\n End program | esc\n\n ENTER: ");
		sc_select = _getch();

		if (sc_select == 's' || sc_select == 'S') {
			system("cls");

			//TCP 방식으로 통신할지 UDP 방식으로 통신할지 선택
			do {
				printf(" Use TCP or UDP\n\n TCP | T or t\n UDP | U or u\n Back | esc\n\n Enter: ");
				type = _getch();

				//TCP 서버 생성
				if (type == 't' || type == 'T') loop = tcp_server();
				//UDP 서버 생성
				else if (type == 'u' || type == 'U') loop = udp_server();
				//뒤로 가기
				else if (type == ESC) {
					system("cls");
					break;
				}
				//잘못된 입력
				else {
					system("cls");
					printf(" WRONG INPUT OCCURED\n");
				}
			} while (loop);
		}
		else if (sc_select == 'c' || sc_select == 'C') {
			system("cls");

			//TCP 방식으로 통신할지 UDP 방식으로 통신할지 선택
			do {
				printf(" Use TCP or UDP\n\n TCP | T or t\n UDP | U or u\n Back | esc\n\n Enter: ");
				type = _getch();

				//TCP 소켓 생성
				if (type == 't' || type == 'T') loop = tcp_client();
				//UDP 소켓 생성
				else if (type == 'u' || type == 'U') loop = udp_client();
				//뒤로가기
				else if (type == ESC) {
					system("cls");
					break;
				}
				//잘못된 입력
				else {
					system("cls");
					printf(" WRONG INPUT OCCURED\n");
				}
			} while (loop);
		}
		else if (sc_select == ESC) {
			break;
		}
		else {
			system("cls");
			printf(" WRONG INPUT OCCURED\n");
		}
	} while (loop);

	printf("\n Program end");
	return 0;
}