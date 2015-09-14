#include "TCPSocket.h"
#include "TcpListener.h"
#include "OpCodes.h"

#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>


int main(int argc, char* argv[])
{
	SocketStartUp();
	int iResult;

	TCPSocket::TCPSocketPtr socket(nullptr);
	TCPListener listener;
	Packet packet;
	UInt8 packetType;
	bool operationCanceled = false;

	char data[DEFAULT_BUFLEN];

	std::vector<std::string> uploadedFiles;

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
		std::cout << "Listener binding failed.\n";
		listener.Close();
		SocketCleanUp();
		return 1;
	}

	while (true)
	{
		if (socket == nullptr || !socket->IsConnected())
		{
			do
			{
				socket.reset(listener.Accept());
			} 
			while (socket == nullptr);
			std::cout << "New client has connected.\n";
		}
		if (!operationCanceled)
			iResult = socket->Receive(packet);
		else
			operationCanceled = false;
		if (iResult == SOCKET_ERROR)
		{
			socket->Close();
			continue;
		}

		packet >> packetType;
		switch (packetType)
		{
			case ECHO_CODE:
			{
				packet >> data;
				std::cout << "Echo command.\nClient says: " << data << std::endl;
				packet.Clear();
				packet << (UInt8)ECHO_CODE << data;
				socket->Send(packet);
				break;

			}
			case TIME_CODE:
			{
				char buff[20];
				time_t now = time(NULL);
				strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
				std::cout << "Time command.\nCurrent time " << buff << std::endl;
				packet.Clear();
				packet << (UInt8)TIME_CODE << buff;
				socket->Send(packet);
				break;
			}
			case CLOSE_CONNECTION_CODE:
			{
				std::cout << "Close command.\nClosing current connection.\n";
				socket->Close();
				break;
			}
			case UPLOAD_CODE:
			{
				std::ofstream uploadingFile;
				char name[20];
				size_t received = 0;
				UInt64 piecesNumber;


				packet >> name >> piecesNumber;

				uploadingFile.open(name, std::fstream::binary | std::fstream::trunc);
				std::cout << piecesNumber << std::endl;
				std::cout << "Receiving " << name << " file.\n";
				for (UInt64 i = 0; i < piecesNumber; i++)
				{
					iResult = socket->Receive(packet);
					if (iResult <= 0)
					{
						std::cout << "Error occuried during uploading. Listening for incoming connections.\n";
						
						socket->Close();
						do
						{
							socket.reset(listener.Accept());
						} 
						while (socket == nullptr);
						socket->Receive(packet);
						packet >> packetType;
						if (packetType != CONTINUE_INTERRUPTED_CODE)
						{
							std::cout << "New client connected.\n";
							operationCanceled = true;
							break;
						}
						else
						{
							std::cout << "Client reconnected.\n";
							i--;
							continue;
						}
					}

					received += iResult;
					UInt32 bytesRead = packet.ExtractRawData(data);
					uploadingFile.write(data, bytesRead);
				}
				uploadingFile.close();
				if (operationCanceled)
				{
					std::cout << "File " << name << " upload was canceled.\n";
					std::remove(name);
				}
				else
				{
					std::cout << "File " << name << " uploaded.\n";
					uploadedFiles.push_back(std::string(name));
				}


				break;
			}
			case DOWNLOAD_CODE:
			{
				std::ifstream downloadingFile;
				char name[20];
				UInt64 size, piecesNumber;

				packet >> name;
				auto iter = std::find(std::begin(uploadedFiles), std::end(uploadedFiles), std::string(name));

				if (iter == std::end(uploadedFiles))
				{
					std::cout << "Attempt to download missing file " << name << std::endl;

					packet.Clear();
					packet << (UInt32)ERROR_CODE;
					iResult = socket->Send(packet);
					break;
				}

				downloadingFile.open(name, std::ifstream::ate | std::ifstream::binary);
				if (!downloadingFile.is_open())
				{
					std::cout << "File opening failed.\n";
					packet.Clear();
					packet << (UInt32)ERROR_CODE;
					iResult = socket->Send(packet);
					break;
				}


				size = downloadingFile.tellg();
				downloadingFile.seekg(0, std::ios::beg);

				piecesNumber = size / DEFAULT_BUFLEN;
				piecesNumber += (size % DEFAULT_BUFLEN) ? 1 : 0;

				packet.Clear();
				packet << (UInt8)CONFIRM_CODE << piecesNumber;
				iResult = socket->Send(packet);
				bool reconnected = false;

				for (UInt64 i = 0; i < piecesNumber; i++)
				{
					UInt32 toRead = DEFAULT_BUFLEN;
					if (!reconnected)
					{
						if (i == piecesNumber - 1)
						{
							toRead = (UInt32)(size - downloadingFile.tellg());
						}
						downloadingFile.read(data, toRead);
					}

					reconnected = false;
					packet.Clear();
					packet.AddRawData(data, toRead);
					iResult = socket->Send(packet);
					if (iResult == SOCKET_ERROR)
					{
						std::cout << "Error occuried during downloading. Listening for incoming connections.\n";
						
						socket->Close();
						do
						{
							socket.reset(listener.Accept());
						} 
						while (socket == nullptr);
						socket->Receive(packet);
						packet >> packetType;
						if (packetType != CONTINUE_INTERRUPTED_CODE)
						{
							operationCanceled = true;
							break;
						}
						else
						{
							std::cout << "Client reconnected.\n";
							i--;
							continue;
						}
					}

				}
				downloadingFile.close();
				if (operationCanceled)
					std::cout << "File download canceled.\n";
				else
					std::cout << "File downloaded.\n";

				break;

			}
			case SHUTDOWN_CODE:
			{
				std::cout << "\nShutdown command.\nShuting down.\n";
				listener.Close();
				socket->Close();
				SocketCleanUp();

				return 0;
			}
		}
	}
}