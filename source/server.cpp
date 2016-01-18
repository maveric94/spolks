#include "TCPSocket.h"
#include "TcpListener.h"
#include "OpCodes.h"
#include "UDPSocket.h"
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

typedef struct 
{
	sockaddr_in *addr;
	bool timeOutEnabled;
} PeerInfo;

std::vector<std::string> uploadedFiles;
TCPListener listener;

enum SocketType
{
	TCP,
	UDP
};


void handlePacket(void *socket, SocketType type, Packet& packet, PeerInfo* peer);

Int32 main(Int32 argc, char* argv[])
{
	SocketStartUp();
	Int32 iResult;
	Int32 tcpTimeout = 0;

	TCPSocket *socket = NULL;
	Packet packet;
	
	UDPSocket *udpSocket = new UDPSocket();
	PeerInfo udpInfo;
	udpInfo.timeOutEnabled = false;
	udpInfo.addr = (sockaddr_in*)malloc(sizeof(sockaddr_in));
	iResult = udpSocket->Bind(SERVER_DEFAULT_UDP_PORT);
	if (iResult != 0)
	{
		std::cout << "Udp socket bind failed.\n";
		SocketCleanUp();
		return 0;
	}


	

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
		if ((socket == NULL || !socket->IsConnected()) && Selector::isListenerReady(&listener, 0))
		{
			std::string address;
			Int32 port;

			std::cout << "Awaiting incoming connections.\n";

			do
			{
				socket = listener.Accept();
			} 
			while (socket == NULL);
			socket->GetSocketRemoteAddress(address, port);
			std::cout << "New client from " << address << " at " << port << " port has connected.\n";
		}

		/*if (!enableTimeout)
		{
			char ch;
			iResult = socket->ReceiveOOBByte(ch);
			if (iResult == 1)
			{
				std::cout << "OOB byte received.\n";
				std::cout << "Total bytes send: " << socket->mBytesSend << "\nTotal bytes received: " << socket->mBytesReceived << std::endl;
			}
		}*/
		if ((socket != NULL && socket->IsConnected()) && Selector::isTCPSocketReadyToReceive(socket, 0))
		{
			iResult = socket->Receive(packet);
			if (iResult > 0)
			{
				handlePacket(socket, TCP, packet, NULL);
				tcpTimeout = 0;
			}

		}
		else
		{
			if (socket != NULL && socket->IsConnected() && socket->isTimeoutEnable)
			{
				tcpTimeout++;
				if (tcpTimeout > 200000)
				{
					socket->Close();
					socket = NULL;
				}
			}
		}

		if (Selector::isUDPSocketReadyToReceive(udpSocket, 0))
		{

            //while (udpSocket->ReceiveFrom(packet, udpInfo.addr) <= 0);

			if (udpSocket->ReceiveFrom(packet, udpInfo.addr) > 0)
				handlePacket(udpSocket, UDP, packet, &udpInfo);
		}
	}
}


void handlePacket(void *socket, SocketType type, Packet& packet, PeerInfo* peer)
{
	UDPSocket* udpSocket = NULL;
	TCPSocket* tcpSocket = NULL;

	if (type == UDP)
		udpSocket = (UDPSocket*)socket;
	else if (type == TCP)
		tcpSocket = (TCPSocket*)socket;

	UInt8 packetType;

	packet >> packetType;
	switch (packetType)
	{
		case HANDSHAKE:
		{
			if (type == TCP)
				tcpSocket->isTimeoutEnable = false;
			else
				peer->timeOutEnabled = false;
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

			if (type == UDP)
				udpSocket->SendTo(packet, peer->addr);
			else if (type == TCP)
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

			if (type == UDP)
				udpSocket->SendTo(packet, peer->addr);
			else if (type == TCP)
				tcpSocket->Send(packet);

			break;
		}
		case CLOSE_CONNECTION:
		{
			std::cout << "Close command.\nClosing current connection.\n";
			if (type == TCP)
			{
				tcpSocket->Close();
			}
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

			//comment this to use server and client on different machines
			//uncomment if you run server and client on one computer
			//also check client.cpp for the same shit

			/*packet << 10;

			if (type == UDP)
				if (Selector::isUDPSocketReadyToSend(udpSocket, 100))
					udpSocket->SendTo(packet, peer->addr);
				else
					std::cout << "UDP client exceeded timeout./n";
			else if (type == TCP)
				if (Selector::isTCPSocketReadyToSend(tcpSocket, 100))
					tcpSocket->Send(packet);
				else
				{
					std::cout << "TCP client dropped due to timeout exceed./n";
					tcpSocket->Close();
				}*/

			//----------------------------------------------------------------

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

			if (type == TCP)
				tcpSocket->isTimeoutEnable = true;
			else
				peer->timeOutEnabled = true;

			packet.Clear();
			packet << LAST_RECEIVED_PIECE << piecesNum - 1;

			if (type == UDP)
				udpSocket->SendTo(packet, peer->addr);
			else if (type == TCP)
				tcpSocket->Send(packet);

			file.close();
			break;
		}
		case FILE_UPLOAD_COMPLETE:
		{
			std::string fileName;

			packet >> fileName;
			uploadedFiles.push_back(fileName);

			if (type == TCP)
				tcpSocket->isTimeoutEnable = false;
			else
				peer->timeOutEnabled = false;
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

			if (type == UDP)
				if (Selector::isUDPSocketReadyToSend(udpSocket, 100))
					udpSocket->SendTo(packet, peer->addr);
				else
					std::cout << "Timeout exceed on UDP client./n";
			else if (type == TCP)
				if (Selector::isTCPSocketReadyToSend(tcpSocket, 100))
					tcpSocket->Send(packet);
				else
				{
					std::cout << "TCP client dropped due to timeout exceed./n";
					tcpSocket->Close();
				}

			if (bytesToRead != DEFAULT_BUFFLEN)
			if (type == TCP)
				tcpSocket->isTimeoutEnable = false;
			else
				peer->timeOutEnabled = false;

			break;
		}
		case SHUTDOWN:
		{
			std::cout << "\nShutdown command.\nShuting down.\n";
			listener.Close();

			if (type == TCP)
				tcpSocket->Close();
			SocketCleanUp();

			exit(0);
		}
		case UPLOAD_REQUEST:
		{
			std::string fileName;
			std::fstream uploadingFile;
			packet >> fileName;

			std::remove(fileName.c_str());
			uploadingFile.open(fileName.c_str());
			uploadingFile.close();

			if (type == TCP)
				tcpSocket->isTimeoutEnable = true;
			else
				peer->timeOutEnabled = true;

			break;
		}
		case DOWNLOAD_REQUEST:
		{
			std::string fileName;
			UInt8 code;
			UInt64 piecesNumber;

			packet >> fileName;

			std::vector<std::string>::iterator iter = std::find(uploadedFiles.begin(), uploadedFiles.end(), fileName);

			if (iter == uploadedFiles.end())
			{
				code = NO_SUCH_FILE;
				piecesNumber = 0;
			}
			else
			{
				std::ifstream downloadingFile;
				UInt64 fileSize;

				downloadingFile.open(iter->c_str(), std::ios::binary | std::ios::ate);
				fileSize = downloadingFile.tellg();
				piecesNumber = fileSize / DEFAULT_BUFFLEN;
				piecesNumber += (fileSize % DEFAULT_BUFFLEN) ? 1 : 0;

				downloadingFile.close();

				code = DOWNLOAD_CONFIRM;

				if (type == TCP)
					tcpSocket->isTimeoutEnable = true;
				else
					peer->timeOutEnabled = true;
			}

			packet.Clear();
			packet << code << piecesNumber;

			if (type == UDP)
				udpSocket->SendTo(packet, peer->addr);
			else if (type == TCP)
				tcpSocket->Send(packet);

			break;
		}
		default:
		{
			std::cout << packetType << " unknown command./n";
		}
	}
}
