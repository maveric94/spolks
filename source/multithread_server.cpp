#include "TCPSocket.h"
#include "TcpListener.h"
#include "OpCodes.h"
#include "Selector.h"

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <stack>

#include <SFML\System\Thread.hpp>
#include <SFML\System\Mutex.hpp>
#include <SFML\System\Lock.hpp>

typedef struct
{
	TCPSocket *socket;
	Int32 threadID;
} ThreadInfo;

const Int32 maxThreadsCount = 10;
const Int32 minThreadsCount = 5;

Int32 currentThreadsCount = 0;
Int32 freeThreadsCount = 0;

std::vector<std::string> uploadedFiles;
std::map<Int32, sf::Thread*> threads;
std::stack<Int32> threadsToClean;

sf::Mutex serverFilesMutex;
sf::Mutex earlyCreatedThreadsMutex;
sf::Mutex currentThreadsCountMutex;
sf::Mutex threadsVaultMutex;
sf::Mutex cleanStackMutex;
sf::Mutex outputMutex;

TCPSocket *socketToHandle = NULL;

void handlePacket(TCPSocket *tcpSocket, Packet& packet);
void threadRoutine(ThreadInfo *info);
void createThread(TCPSocket *socket);

Int32 getFreeKey()
{
	while (true)
	{
		std::map<Int32, sf::Thread*>::iterator it;
		Int32 newKey = rand();

		it = threads.find(newKey);
		if (it == threads.end())
		{
			return newKey;
		}
	}
}

void consoleOut(char* format, const char* param1, Int32 param2)
{
	outputMutex.lock();
	printf(format, param1, param2);
	outputMutex.unlock();
}
void consoleOut(char* format, Int32 param)
{
	outputMutex.lock();
	printf(format, param);
	outputMutex.unlock();
}
void consoleOut(const char* format)
{
	outputMutex.lock();
	printf(format);
	outputMutex.unlock();
}

Int32 main(Int32 argc, char* argv[])
{
	SocketStartUp();
	Int32 iResult;
	TCPListener listener;

	earlyCreatedThreadsMutex.lock();

	iResult = listener.Bind(((argc == 2) ? argv[1] : DEFAULT_PORT));
	if (iResult != 0)
	{
		std::cout << "Listener binding failed.\n";
		SocketCleanUp();
		return 1;
	}

	iResult = listener.Listen();
	if (iResult != 0)
	{
		std::cout << "Listen failed.\n";
		listener.Close();
		SocketCleanUp();
		return 1;
	}

	while (true)
	{
		
		while (currentThreadsCount < minThreadsCount)
		{
			createThread(NULL);
		}

		if (Selector::isListenerReady(&listener, 100))
		{
			TCPSocket *socket = NULL;

			currentThreadsCountMutex.lock();
			if (freeThreadsCount == 0 || currentThreadsCount >= maxThreadsCount)
			{
				currentThreadsCountMutex.unlock();
				consoleOut("Server: not enough threads to handle new client.\n");
				continue;
			}
			currentThreadsCountMutex.unlock();

			do
			{
				socket = listener.Accept();
			} 
			while (socket == NULL);

			std::string address;
			Int32 port;
			socket->GetSocketRemoteAddress(address, port);
			consoleOut("Server: new client from %s at %d port has connected.\n", address.c_str(), port);

			if (freeThreadsCount > 0)
			{
				consoleOut("Server: unlocking new socket for early created threads.\n");
				freeThreadsCount--;
				socketToHandle = socket;

				earlyCreatedThreadsMutex.unlock();
				sleep(50);
				earlyCreatedThreadsMutex.lock();
			}
			else
			{
				createThread(socket);
			}

			
		}


		cleanStackMutex.lock();
		threadsVaultMutex.lock();
		while (threadsToClean.size() != 0)
		{
			Int32 threadID = threadsToClean.top();
			threads.erase(threadID);
			threadsToClean.pop();

			consoleOut("Server: thread %d is cleaned up.\n", threadID);
		}
		threadsVaultMutex.unlock();
		cleanStackMutex.unlock();
		

	}
}

void createThread(TCPSocket *socket)
{
	currentThreadsCountMutex.lock();
	threadsVaultMutex.lock();


	Int32 newKey = getFreeKey();

	ThreadInfo *info = new ThreadInfo();
	info->socket = socket;
	info->threadID = newKey;


	threads[newKey] = new sf::Thread(threadRoutine, info);
	threads[newKey]->launch();

	currentThreadsCount++;
	if (socket == NULL)
	{
		consoleOut("Server: early thread %d is created.\n", newKey);
		freeThreadsCount++;
	}
	else
	{
		consoleOut("Server: thread %d is created with socket.\n", newKey);
	}

	currentThreadsCountMutex.unlock();
	threadsVaultMutex.unlock();
}

void threadRoutine(ThreadInfo *info)
{
	TCPSocket *socket = info->socket;
	Int32 iResult, tcpTimeout = 0;
	Packet packet;

	while (socket == NULL)
	{
		consoleOut("Thread %d: trying to capture socket.\n", info->threadID);

		earlyCreatedThreadsMutex.lock();
		socket = socketToHandle;
		socketToHandle = NULL;
		earlyCreatedThreadsMutex.unlock();
	}

	consoleOut("Thread %d: socket is captured.\n", info->threadID);

	while (true)
	{
		if ((socket != NULL && socket->IsConnected()) && Selector::isTCPSocketReadyToReceive(socket, 0))
		{
			iResult = socket->Receive(packet);
			if (iResult > 0)
			{
				handlePacket(socket, packet);
				tcpTimeout = 0;
			}

		}
		else if (socket != NULL && socket->IsConnected() && socket->isTimeoutEnable)
		{
			tcpTimeout++;
			if (tcpTimeout > 200000)
			{
				socket->Close();
				socket = NULL;
				break;
			}
		}
		else if (socket != NULL && !socket->IsConnected())
			break;
	
	}

	consoleOut("Thread %d: finishing thread.\n", info->threadID);
	cleanStackMutex.lock();
	currentThreadsCountMutex.lock();

	currentThreadsCount--;
	threadsToClean.push(info->threadID);
	delete info;

	currentThreadsCountMutex.unlock();
	cleanStackMutex.unlock();
	//handle exit here
}


void handlePacket(TCPSocket *tcpSocket, Packet& packet)
{
	UInt8 packetType;

	packet >> packetType;
	switch (packetType)
	{
	case HANDSHAKE:
	{
		tcpSocket->isTimeoutEnable = false;

		break;
	}
	case ECHO:
	{
		std::string msg;
		packet >> msg;
		std::cout << "Echo command.\nClient says: " << msg << std::endl;
		packet.Clear();

		if (msg == "Hustone")
			msg = "We have a problem.";

		packet << ECHO << msg;


		tcpSocket->Send(packet);

		break;

	}
	case TIME:
	{
		char buff[20];
		time_t now = time(NULL);

		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

		std::cout << "Time command.\nCurrent time " << buff << std::endl;

		packet.Clear();
		packet << TIME << buff;

		tcpSocket->Send(packet);

		break;
	}
	case CLOSE_CONNECTION:
	{
		std::cout << "Close command.\nClosing current connection.\n";

		tcpSocket->Close();

		break;
	}
	case UPLOAD_PIECE:
	{
		UInt32 size;
		UInt64 pieceNumber;
		std::string fileName;
		std::ofstream file;

		packet >> fileName >> pieceNumber >> size;

		file.open(fileName.c_str(), std::ios::binary | std::ios::app);

		char* piece = new char[size];
		packet.ExtractRawData(piece);

		file.seekp(pieceNumber * DEFAULT_BUFFLEN, std::ios_base::beg);
		file.write(piece, size);
		delete piece;
		file.close();

		packet.Clear();
		/*packet << 10;

		if (Selector::isTCPSocketReadyToSend(tcpSocket, 100))
			tcpSocket->Send(packet);
		else
		{
			std::cout << "TCP client dropped due to timeout exceed./n";
			tcpSocket->Close();
		}*/

		break;
	}
	case CONTINUE_INTERRUPTED_UPLOAD:
	{
		std::ifstream file;
		std::string fileName;
		UInt64 fileSize, piecesNum;

		packet >> fileName;

		file.open(fileName.c_str(), std::ios::ate | std::ios::binary);
		fileSize = file.tellg();
		piecesNum = fileSize / DEFAULT_BUFFLEN;


		tcpSocket->isTimeoutEnable = true;


		packet.Clear();
		packet << LAST_RECEIVED_PIECE << piecesNum - 1;


		tcpSocket->Send(packet);

		file.close();
		break;
	}
	case FILE_UPLOAD_COMPLETE:
	{
		std::string fileName;

		packet >> fileName;

		serverFilesMutex.lock();
		uploadedFiles.push_back(fileName);
		serverFilesMutex.unlock();


		tcpSocket->isTimeoutEnable = false;

		break;
	}
	case DOWNLOAD_PIECE:
	{
		/*bool downloadStopped = false;
		UInt64 pieceNumber, fileSize, piecesNumber;
		char* piece = new char[DEFAULT_BUFFLEN];

		packet >> pieceNumber;

		downloadingFile.seekg(0, std::ios_base::end);
		fileSize = downloadingFile.tellg();

		piecesNumber = fileSize / DEFAULT_BUFFLEN;
		piecesNumber += (fileSize % DEFAULT_BUFFLEN) ? 1 : 0;

		for (UInt64 i = pieceNumber; i < piecesNumber; i++)
		{
		UInt32 bytesToRead = (i == piecesNumber - 1) ? ((fileSize % DEFAULT_BUFFLEN) ? fileSize % DEFAULT_BUFFLEN : DEFAULT_BUFFLEN) : DEFAULT_BUFFLEN;

		downloadingFile.seekg(i * DEFAULT_BUFFLEN);
		downloadingFile.read(piece, bytesToRead);

		packet.Clear();
		packet << DOWNLOAD_PIECE << i << bytesToRead;
		packet.AddRawData(piece, bytesToRead);

		iResult = socket->Send(packet, defaultTimeout);
		if (iResult <= 0)
		{
		downloadStopped = true;
		break;
		}
		}

		if (downloadStopped)
		{
		std::cout << "Closing connection due to timeout exceed.\n";
		socket->Close();
		break;
		}

		packet.Clear();
		packet << FILE_DOWLOAD_COMPLETE;
		socket->Send(packet);*/

		UInt64 pieceNumber, fileSize, piecesNumber;
		std::string filename;
		std::ifstream file;
		char *piece = new char[DEFAULT_BUFFLEN];

		packet >> filename >> pieceNumber;

		file.open(filename.c_str(), std::ios::binary | std::ios::ate);
		fileSize = file.tellg();
		piecesNumber = fileSize / DEFAULT_BUFFLEN;
		file.seekg(pieceNumber * DEFAULT_BUFFLEN);

		UInt32 bytesToRead = (pieceNumber == piecesNumber) ? ((fileSize % DEFAULT_BUFFLEN) ? fileSize % DEFAULT_BUFFLEN : DEFAULT_BUFFLEN) : DEFAULT_BUFFLEN;
		file.read(piece, bytesToRead);
		file.close();

		packet.Clear();
		packet << DOWNLOAD_PIECE << pieceNumber << bytesToRead;
		packet.AddRawData(piece, bytesToRead);

		if (Selector::isTCPSocketReadyToSend(tcpSocket, 100))
			tcpSocket->Send(packet);
		else
		{
			std::cout << "TCP client dropped due to timeout exceed./n";
			tcpSocket->Close();
		}

		if (bytesToRead != DEFAULT_BUFFLEN)
			tcpSocket->isTimeoutEnable = false;


		break;
	}
	case SHUTDOWN:
	{
		std::cout << "\nShutdown command.\nShuting down.\n";
		tcpSocket->Close();
	}
	case UPLOAD_REQUEST:
	{
		std::string fileName;
		std::fstream uploadingFile;
		packet >> fileName;

		std::remove(fileName.c_str());
		uploadingFile.open(fileName.c_str());
		uploadingFile.close();

		tcpSocket->isTimeoutEnable = true;


		break;
	}
	case DOWNLOAD_REQUEST:
	{
		std::string fileName;
		UInt8 code;
		UInt64 piecesNumber;

		packet >> fileName;

		serverFilesMutex.lock();
		std::vector<std::string>::iterator iter = std::find(uploadedFiles.begin(), uploadedFiles.end(), fileName);

		if (iter == uploadedFiles.end())
		{
			serverFilesMutex.unlock();
			code = NO_SUCH_FILE;
			piecesNumber = 0;
		}
		else
		{
			serverFilesMutex.unlock();
			std::ifstream downloadingFile;
			UInt64 fileSize;

			downloadingFile.open(iter->c_str(), std::ios::binary | std::ios::ate);
			fileSize = downloadingFile.tellg();
			piecesNumber = fileSize / DEFAULT_BUFFLEN;
			piecesNumber += (fileSize % DEFAULT_BUFFLEN) ? 1 : 0;

			downloadingFile.close();

			code = DOWNLOAD_CONFIRM;

			tcpSocket->isTimeoutEnable = true;
	
		}

		packet.Clear();
		packet << code << piecesNumber;

		tcpSocket->Send(packet);

		break;
	}
	default:
	{
		 std::cout << packetType << " unknown command./n";
	}
	}
}
