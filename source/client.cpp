#include "TCPSocket.h"
#include "UDPSocket.h"
#include "Selector.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <string>

#include "OpCodes.h"



Int32 main(Int32 argc, char* argv[])
{
	std::string proto, addr;
	bool isUDP = true;
	std::cout << "Address: ";
	std::cin >> addr;
	std::cout << "Protocol: ";
	std::cin >> proto;

	if (proto == "tcp")
	{
		isUDP = false;
	}
	else if (proto == "udp")
	{
		isUDP = true;
	}
	else
	{
		std::cout << "Unknow protocol.";
		return 0;
	}

	SocketStartUp();
	Packet packet;
	char data[DEFAULT_BUFFLEN];
	Int32 iResult;

	std::string str;
	TCPSocket socket;

	UDPSocket udpSocket;

	sockaddr_in to, from;

	if (!isUDP)
	{
		iResult = socket.Connect(addr.c_str(), (argc == 4) ? argv[3] : DEFAULT_PORT);
		if (iResult != 0)
		{
			std::cout << "Connecting to server failed.\n";
			socket.Close();
			SocketCleanUp();
			return 1;
		}
	}
	else
	{
		iResult = udpSocket.Bind(CLIENT_DEFAULT_UDP_PORT);
		if (iResult != 0)
		{
			std::cout << "Udp socket bind failed.\n";
			SocketCleanUp();
			return 0;
		}
		getSockaddr(&to, addr.c_str(), ServerUDPPort);
	}

	std::cout << "Connection to server was succesesful.\n";

	packet.Clear();
	packet << HANDSHAKE;
	if (isUDP)
		udpSocket.SendTo(packet, &to);
	else
		socket.Send(packet);
	
	while (true)
	{
		std::cin >> str;
		if (str == "upload")
		{
			std::ifstream uploadingFile;
			UInt64 size, piecesNumber;
			std::string name;

			std::cin.ignore(1);
			std::getline(std::cin, name);

			uploadingFile.open(name.c_str(), std::ifstream::ate | std::ifstream::binary);
			if (!uploadingFile.is_open())
			{
				std::cout << "Incorrect name of file: " << name << std::endl;
				continue;
			}

			size = uploadingFile.tellg();
			uploadingFile.seekg(0, std::ios::beg);

			piecesNumber = size / DEFAULT_BUFFLEN;
			piecesNumber += (size % DEFAULT_BUFFLEN) ? 1 : 0;

			std::cout << "Uploading file: " << name << ". Size " << size << " bytes.\n";

			packet.Clear();
			packet << UPLOAD_REQUEST << name;

			if (isUDP)
				udpSocket.SendTo(packet, &to);
			else
				socket.Send(packet);

			char* piece = new char[DEFAULT_BUFFLEN];

			for (UInt64 i = 0; i < piecesNumber; i++)
			{
				/*if (i == piecesNumber / 2)
				{
					socket.SendOOBByte(0x00);
					std::cout << "OOB byte is sent.\n";
				}*/

				UInt32 bytesToRead = (i == piecesNumber - 1) ? ((size % DEFAULT_BUFFLEN) ? size % DEFAULT_BUFFLEN : DEFAULT_BUFFLEN) : DEFAULT_BUFFLEN;
				
				uploadingFile.seekg(i * DEFAULT_BUFFLEN);
				uploadingFile.read(piece, bytesToRead);

				packet.Clear();
				packet << UPLOAD_PIECE << name << i << bytesToRead;
				packet.AddRawData(piece, bytesToRead);

				if (isUDP)
				{
					if (Selector::isUDPSocketReadyToSend(&udpSocket, 5000))
					{
						//sleep(10);
						iResult = udpSocket.SendTo(packet, &to);
					}
					else
						iResult = -1;
				}
				else
					iResult = socket.Send(packet, defaultTimeout);

				if (iResult <= 0)
				{
					errorHandle:
					std::cout << "Timeout exceeding. Trying to reconnect to server.\n";
					if (!isUDP)
					{
						socket.Close();
						for (UInt32 j = 0; j < 100; j++)
						{
							socket.Reconnect();
							if (socket.IsConnected())
								break;
							sleep(100);
						}

						if (socket.IsConnected())
						{
							packet.Clear();
							packet << CONTINUE_INTERRUPTED_UPLOAD << name;
							socket.Send(packet);

							socket.Receive(packet);

							UInt8 code;
							UInt64 pieceNumber;

							packet >> code >> pieceNumber;
							i = pieceNumber;
							continue;
						}
						else
						{
							std::cout << "Reconnection failed.\n";
							socket.Close();
							SocketCleanUp();
							delete piece;
							return 1;
						}
					}
					else
					{
						Int32 c;
						for (c = 0; c < 10; c++)
						{
							if (Selector::isUDPSocketReadyToSend(&udpSocket, 1000))
								break;
						}
						if (c == 9)
						{
							std::cout << "Reconnection failed.\n";
							socket.Close();
							SocketCleanUp();
							delete piece;
							return 1;
						}
						packet.Clear();
						packet << CONTINUE_INTERRUPTED_UPLOAD << name;
						udpSocket.SendTo(packet, &to);

						udpSocket.ReceiveFrom(packet, &from);

						UInt8 code;
						UInt64 pieceNumber;

						packet >> code >> pieceNumber;
						i = pieceNumber;
						continue;
					}
					
				}

				//comment this to use server and client on different machines
				//uncomment if you run server and client on one computer
				//also check server.cpp for the same shit

				/*if (isUDP)
				{
					if (Selector::isUDPSocketReadyToReceive(&udpSocket, 5000))
						iResult = udpSocket.ReceiveFrom(packet, &from);
					else
						iResult = -1;
				}
				else
					iResult = socket.Receive(packet, defaultTimeout);

				if (iResult < 0)
					goto errorHandle;*/

				//-----------------------------------------------------------

			}
			
			packet.Clear();
			packet << FILE_UPLOAD_COMPLETE << name;

			if (isUDP)
				udpSocket.SendTo(packet, &to);
			else
				socket.Send(packet);

			uploadingFile.close();

			std::cout << "Uploading file: " << name << " complete.\n";

			delete piece;

		}
		else if (str == "download")
		{
			std::ofstream downloadingFile;
			UInt8 code;
			UInt64 piecesNumber;
			std::string name;
			std::cin.ignore(1);
			std::getline(std::cin, name);

			packet.Clear();
			packet << DOWNLOAD_REQUEST << name;
			if (isUDP)
				udpSocket.SendTo(packet, &to);
			else
				socket.Send(packet);

			if (isUDP)
				udpSocket.ReceiveFrom(packet, &from);
			else
				socket.Receive(packet);
			packet >> code >> piecesNumber;

			if (code == NO_SUCH_FILE)
			{
				std::cout << "No such file on server.\n";
				continue;
			}

			downloadingFile.open(name.c_str(), std::fstream::binary | std::fstream::trunc);
			if (!downloadingFile.is_open())
			{
				std::cout << "File creation failed.\n";
				break;
			}

			UInt64 lastDownloadedPiece = 0;

			for (UInt64 i = 0; i < piecesNumber; i++)
			{
				packet.Clear();
				packet << DOWNLOAD_PIECE << name << i;
				if (isUDP)
				{
					if (Selector::isUDPSocketReadyToSend(&udpSocket, 5000))
						iResult = udpSocket.SendTo(packet, &to);
					else
						iResult = -1;
				}
				else
					iResult = socket.Send(packet, defaultTimeout);
				if (iResult <= 0)
					goto errorHandling;

				if (isUDP)
				{
					if (Selector::isUDPSocketReadyToReceive(&udpSocket, 5000))
						iResult = udpSocket.ReceiveFrom(packet, &from);
					else
						iResult = -1;
				}
				else
					iResult = socket.Receive(packet, defaultTimeout);
				if (iResult <= 0)
				{
					errorHandling:
					std::cout << "Error occuried during downloading. Trying to reconnect.\n";

					if (!isUDP)
					{
						for (UInt32 j = 0; j < 100; j++)
						{
							sleep(100);
							socket.Reconnect();
							if (socket.IsConnected())
								break;
						}

						if (socket.IsConnected())
						{
							std::cout << "Reconnection was successeful.\n";
							i--;
							continue;
						}
						else
						{
							std::cout << "Reconnection failed\n.";
							socket.Close();
							downloadingFile.close();
							SocketCleanUp();
							return 1;
						}
					}
					else
					{
						Int32 i;
						for (i = 0; i < 10; i++)
						{
							if (Selector::isUDPSocketReadyToSend(&udpSocket, 1000))
								break;
						}
						if (i == 9)
						{
							std::cout << "Reconnection failed\n.";
							socket.Close();
							downloadingFile.close();
							SocketCleanUp();
							return 1;
						}

						std::cout << "Reconnection was successeful.\n";
						i--;
						continue;
					}
				}
				UInt8 code;
				UInt64 pieceNumber;
				UInt32 size;

				packet >> code;

				if (code == DOWNLOAD_PIECE)
				{
					packet >> pieceNumber >> size;

					char* piece = new char[size];
					packet.ExtractRawData(piece);

					downloadingFile.seekp(pieceNumber * DEFAULT_BUFFLEN);
					downloadingFile.write(piece, size);

					lastDownloadedPiece = pieceNumber;
				}
				else if (code == FILE_DOWLOAD_COMPLETE)
				{
					break;
				}

			}
			downloadingFile.close();
			std::cout << "File downloaded.\n";
		}
		else if (str == "close")
		{
			packet.Clear();
			packet << (UInt8)CLOSE_CONNECTION;
			if (isUDP)
				iResult = udpSocket.SendTo(packet, &to);
			else
				iResult = socket.Send(packet);
			std::cout << "Close command.\n" << packet.Size() << " bytes are sent.\n";
			break;
		}
		else if (str == "shutdown")
		{
			std::cout << "Shutdown command.\n";
			packet.Clear();
			packet << (UInt8)SHUTDOWN;
			socket.Send(packet);
			break;
		}
		else if (str == "echo")
		{
			std::string msg;
			std::string recv;

			std::cin.ignore(1);
			std::getline(std::cin, msg);

			packet.Clear();
			packet << (UInt8)ECHO << msg;

			if (isUDP)
			{
				udpSocket.SendTo(packet, &to);
				udpSocket.ReceiveFrom(packet, &from);
			}
			else
			{
				socket.Send(packet);
				socket.Receive(packet);
			}

			UInt8 type;
			packet >> type >> recv;
			std::cout << "Server says: " << recv << std::endl;
		}
		else if (str == "time")
		{
			std::cout << "Time command.\n";
			packet.Clear();
			packet << (UInt8)TIME;
			if (isUDP)
			{
				udpSocket.SendTo(packet, &to);
                udpSocket.ReceiveFrom(packet, &to);
			}
			else
			{
				iResult = socket.Send(packet);
				iResult = socket.Receive(packet);
			}
			UInt8 type;
			packet >> type >> data;
			std::cout << "Server says: " << data << std::endl;
		}
		else
			std::cout << "Unknown command.\n";
	}

	socket.Close();
	SocketCleanUp();
	return 0;
}

