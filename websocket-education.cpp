#include <iostream>
#include <sstream>
#include <string>

#define _WIN32_WINNT 0x501

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using std::cerr;

int main() {
	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result != 0) {
		cerr << "WSAStartup failed: " << result << '\n';
		return result;
	}

	struct addrinfo* addr = NULL;

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	// используется сеть для работы с сокетом
	hints.ai_family = AF_INET;
	// потоковый тип сокета
	hints.ai_socktype = SOCK_STREAM;
	// TCP
	hints.ai_protocol = IPPROTO_TCP;

	// биндим сокет на адрес, чтобы принимать входящие соединения 
	hints.ai_flags = AI_PASSIVE;

	result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);

	if (result != 0) {
		cerr << "getaddrinfo failed: " << result << '\n';
		// выгрузка (удаление) библиотеки Ws2_32.dll
		WSACleanup();
		return 1;
	}

	// СОЗДАНИЕ СОКЕТА . . .			

	int listen_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

	// сокет не создался
	if (listen_socket == INVALID_SOCKET) {
		cerr << "Error at socket: " << WSAGetLastError() << '\n';
		// освобождение памяти, затронутой под структуру 
		freeaddrinfo(addr);
		// выгрузка (удаление) библиотеки Ws2_32.dll
		WSACleanup();
		return 1;
	}

	// СОКЕТ СОЗДАЛСЯ . . .

	// Привязываем сокет к адресу 
	result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

	// не удалось привязать сокет к адресу
	if (result == SOCKET_ERROR) {
		cerr << "bind failed with error: " << WSAGetLastError() << '\n';
		// освобождение памяти, затронутой под структуру
		freeaddrinfo(addr);
		// выгрузка (удаление) библиотеки Ws2_32.dll
		WSACleanup();
		return 1;
	}

	// СОКЕТ ПРИВЯЗАН К АДРЕСУ СЕТИ . . .

	// listen(socket, SOMAXCONN) == (сокет, макс_колличество_подключений)
	// SOMAXCONN - константа, работает на уровне ядра
	if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "listen failed with error: " << WSAGetLastError() << '\n';
		// закрыть созданный сокет
		closesocket(listen_socket);
		// выгрузить библиотеку
		WSACleanup();
		return 1;
	}

	// БЕСКОНЕЧНАЯ ОБРАБОТКА ЗАПРОСОВ 	
	for (;;) { 
		// Принимаем входящее соединение
		int client_socket = accept(listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			cerr << "accept failed with error: " << WSAGetLastError() << '\n';
			// закрыть сокет
			closesocket(listen_socket);
			// выгрузить библиотеку
			WSACleanup();
			return 1;
		}

		const int max_client_buffer_size = 1024; // байта
		char buf[max_client_buffer_size];

		// возвращает кол-во байт в ответе (0, если соединение закрыто клиентом)
		// SOCKET_ERROR, если ошибка
		// иначе - кол-во байт в ответе 
		result = recv(client_socket, buf, max_client_buffer_size, 0);

		// сюда записывает ответ клиенту
		std::stringstream response;
		// тело ответа
		std::stringstream response_body;

		if (result == SOCKET_ERROR) {
			cerr << "recv failed with error: " << result << '\n';
			// закрыть открытый сокет
			closesocket(listen_socket);
		}
		else if (result == 0) {
			cerr << "connection closed..." << '\n';
		}
		else if (result > 0) {
			// мы знаем фактический размер полученных данных,
			// ставим метку конца строки в буфере запроса
			buf[result] = '\0';

			// данные успешно полуены!
			// формируем тело ответа (html)
			response_body << "<title>C++ HTTP Server</title>\n"
				<< "<h1>Test page</h1>\n"
				<< "<p>This is body of the test page...</p>\n"
				<< "<h2>Request headers</h2>\n"
				<< "<pre>" << buf << "</pre>\n"
				<< "<em><small>C++ HTTP Server</small></em>\n";

			// весь ответ вместе с html 
			response << "HTTP/1.1 200 OK\r\n"
				<< "Version: HTTP/1.1\r\n"
				<< "Content-Type: text/html; charset=utf-8\r\n"
				<< "Content-Length: " << response_body.str().length()
				<< "\r\n\r\n"
				<< response_body.str();

			// отправляем результат клиенту
			result = send(client_socket, response.str().c_str(), response.str().length(), 0);
			if (result == SOCKET_ERROR) {
				cerr << "send failed with errro: " << WSAGetLastError() << '\n';
			}
			// закрыть сокет (все хорошо)
			closesocket(client_socket);
		}
	}

	// ВЫПОЛНИТСЯ ТОЛЬКО ЕСЛИ УБРАТЬ ЦИКЛ FOR 
	// ИЛИ ЗАВЕРШИТЬ ЦИКЛ каким-то образом

	// убираем за собой! (это важно в C++)
	// закрыть сокет сервера
	closesocket(listen_socket);
	// освободить память от структур
	freeaddrinfo(addr);
	// выгрузить библиотеку
	WSACleanup();
	return 0;
}