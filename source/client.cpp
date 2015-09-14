#include "TCPSocket.h"
#include "Socket.h"
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <string>

#include "OpCodes.h"



int main(int argc, char* argv[])
{
	/*if (argc < 2)
	{
		std::cout << "Pass a IP as a command line argument.";
		return 0;
	}*/

	SocketStartUp();
	TCPSocket socket;
	Packet packet;
	char data[DEFAULT_BUFLEN];
	int iResult;

	std::string str;

	iResult = socket.Connect("127.0.0.1", (argc == 3) ? argv[2] : DEFAULT_PORT);
	if (iResult != 0)
	{
		std::cout << "Connecting to server failed./n";
		socket.Close();
		SocketCleanUp();
		return 1;
	}
	

	while (true)
	{
		std::cin >> str;
		if (str == "upload")
		{
			std::ifstream uploadingFile;
			UInt64 size, piecesNumber;
			char name[20];
			std::cin.ignore(1);
			std::cin.getline(name, 20);


			uploadingFile.open(name, std::ifstream::ate | std::ifstream::binary);
			if (!uploadingFile.is_open())
			{
				std::cout << "Incorrect name of file: " << name << std::endl;
				continue;
			}

			size = uploadingFile.tellg();
			uploadingFile.seekg(0, std::ios::beg);

			piecesNumber = size / DEFAULT_BUFLEN;
			piecesNumber += (size % DEFAULT_BUFLEN) ? 1 : 0;

			packet.Clear();
			packet << (UInt8)UPLOAD_CODE << name << piecesNumber;

			std::cout << "Uploading file " << size << std::endl;

			//Send an initial data
			iResult = socket.Send(packet);
			if (iResult == SOCKET_ERROR)
			{
				std::cout << "Sending data  error.\n";
				socket.Close();
				SocketCleanUp();
				return 1;
			}
			bool reconnected = false;
			for (UInt64 i = 0; i < piecesNumber; i++)
			{
				UInt32 toRead = DEFAULT_BUFLEN;
				/*if (i == 10 && !reconnected)
					socket.Close();*/
				if (!reconnected)
				{
					if (i == piecesNumber - 1)
					{
						toRead = (UInt32)(size - uploadingFile.tellg());
					}
					uploadingFile.read(data, toRead);
				}

				reconnected = false;
				packet.Clear();
				packet.AddRawData(data, toRead);
				iResult = socket.Send(packet);
				if (iResult == SOCKET_ERROR)
				{
					std::cout << "Error occuried during uploading. Trying to reconnect.\n";
					socket.Close(); //just in case
					for (UInt32 j = 0; j < 100; j++)
					{
						sleep(100);
						iResult = socket.Reconnect();
						if (iResult == 0)
							break;
					}
					if (iResult != 0)
					{
						std::cout << "Reconnection failed\n.";
						socket.Close();
						uploadingFile.close();
						SocketCleanUp();
						return 1;
					}
					else
					{
						std::cout << "Reconnection was successeful.\n";
						packet.Clear();
						packet << (UInt8)CONTINUE_INTERRUPTED_CODE;
						socket.Send(packet);
						reconnected = true;
						i--;
						continue;
					}
				}

			}
			uploadingFile.close();
			std::cout << "File " << name << " uploaded.\n";

		}
		else if (str == "download")
		{
			std::ofstream downloadingFile;
			UInt64 piecesNumber;
			UInt8 opCode;
			char name[20];
			std::cin.ignore(1);
			std::cin.getline(name, 20);

			packet.Clear();
			packet << (UInt8)DOWNLOAD_CODE << name;
			iResult = socket.Send(packet);
			if (iResult == SOCKET_ERROR)
			{
				//make smth later
			}

			iResult = socket.Receive(packet);
			if (iResult == SOCKET_ERROR)
			{
				//make smth later
			}

			packet >> opCode;
			if (opCode == ERROR_CODE)
			{
				std::cout << "No such file on server.\n";
				continue;
			}
			else if (opCode == CONFIRM_CODE)
			{
				packet >> piecesNumber;
			}

			downloadingFile.open(name, std::fstream::binary | std::fstream::trunc);
			if (!downloadingFile.is_open())
			{
				std::cout << "File creation failed.\n";
				break;
			}

			for (UInt64 i = 0; i < piecesNumber; i++)
			{
				iResult = socket.Receive(packet);
				if (iResult == SOCKET_ERROR)
				{
					std::cout << "Error occuried during downloading. Trying to reconnect.\n";

					for (UInt32 j = 0; j < 100; j++)
					{
						sleep(100);
						iResult = socket.Reconnect();
						if (iResult == 0)
							break;
					}

					if (iResult != 0)
					{
						std::cout << "Reconnection failed\n.";
						socket.Close();
						downloadingFile.close();
						SocketCleanUp();
						return 1;
					}
					else
					{
						std::cout << "Reconnection was successeful.\n";
						packet.Clear();
						packet << (UInt8)CONTINUE_INTERRUPTED_CODE;
						socket.Send(packet);
						i--;
						continue;
					}
				}
				UInt32 bytesRead = packet.ExtractRawData(data);
				downloadingFile.write(data, bytesRead);
			}
			downloadingFile.close();
			std::cout << "File downloaded.\n";


		}
		else if (str == "close")
		{
			packet.Clear();
			packet << (UInt8)CLOSE_CONNECTION_CODE;
			iResult = socket.Send(packet);
			std::cout << "Close command.\n" << packet.Size() << " bytes are sent.\n";
			break;
		}
		else if (str == "shutdown")
		{
			std::cout << "Shutdown command.\n";
			packet.Clear();
			packet << (UInt8)SHUTDOWN_CODE;
			iResult = socket.Send(packet);
			break;
		}
		else if (str == "echo")
		{
			std::cin.ignore(1);
			std::cin.getline(data, DEFAULT_BUFLEN);
			packet.Clear();
			packet << (UInt8)ECHO_CODE << data;
			iResult = socket.Send(packet);
			iResult = socket.Receive(packet);
			UInt8 type;
			packet >> type >> data;
			std::cout << "Server says: " << data << std::endl;
		}
		else if (str == "time")
		{
			std::cout << "Time command.\n";
			packet.Clear();
			packet << (UInt8)TIME_CODE;
			iResult = socket.Send(packet);
			iResult = socket.Receive(packet);
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

