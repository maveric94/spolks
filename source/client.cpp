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
	char data[DEFAULT_BUFFLEN];
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

	std::cout << "Connection to server was succesesful.\n";

	packet.Clear();
	packet << HANDSHAKE;
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

			uploadingFile.open(name, std::ifstream::ate | std::ifstream::binary);
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

			socket.Send(packet);

			char* piece = new char[DEFAULT_BUFFLEN];

			for (UInt64 i = 0; i < piecesNumber; i++)
			{
				UInt32 bytesToRead = (i == piecesNumber - 1) ? size % DEFAULT_BUFFLEN : DEFAULT_BUFFLEN;
				
				uploadingFile.seekg(i * DEFAULT_BUFFLEN);
				uploadingFile.read(piece, bytesToRead);

				packet.Clear();
				packet << UPLOAD_PIECE << i << bytesToRead;
				packet.AddRawData(piece, bytesToRead);

				iResult = socket.Send(packet, 5);
				if (iResult <= 0)
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
						packet << CONTINUE_INTERRUPTED_UPLOAD;
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

			}
			
			packet.Clear();
			packet << FILE_UPLOAD_COMPLETE;

			socket.Send(packet);

			uploadingFile.close();

			std::cout << "Uploading file: " << name << " complete.\n";

			delete piece;

		}
		else if (str == "download")
		{
			std::ofstream downloadingFile;
			UInt8 code;
			std::string name;
			std::cin.ignore(1);
			std::getline(std::cin, name);

			packet.Clear();
			packet << DOWNLOAD_REQUEST << name;
			socket.Send(packet);

			socket.Receive(packet);
			packet >> code;

			if (code == NO_SUCH_FILE)
			{
				std::cout << "No such file on server.\n";
				continue;
			}

			downloadingFile.open(name, std::fstream::binary | std::fstream::trunc);
			if (!downloadingFile.is_open())
			{
				std::cout << "File creation failed.\n";
				break;
			}

			packet.Clear();
			packet << DOWNLOAD_PIECE << (UInt64)0;
			socket.Send(packet);

			UInt64 lastDownloadedPiece = 0;

			while (true)
			{
				iResult = socket.Receive(packet, 5);
				if (iResult <= 0)
				{
					std::cout << "Error occuried during downloading. Trying to reconnect.\n";

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
						packet.Clear();
						packet << DOWNLOAD_PIECE << lastDownloadedPiece;
						socket.Send(packet);
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

			socket.Send(packet);
			socket.Receive(packet);

			UInt8 type;
			packet >> type >> recv;
			std::cout << "Server says: " << recv << std::endl;
		}
		else if (str == "time")
		{
			std::cout << "Time command.\n";
			packet.Clear();
			packet << (UInt8)TIME;
			iResult = socket.Send(packet);
			iResult = socket.Receive(packet);
			UInt8 type;
			packet >> type >> data;
			std::cout << "Server says: " << data << std::endl;
		}
		else if (str == "stat")
		{
			char a = 'a';
			socket.SendOOBByte(a);
		}
		else
			std::cout << "Unknown command.\n";
	}

	socket.Close();
	SocketCleanUp();
	return 0;
}

