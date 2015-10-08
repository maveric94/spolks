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
	UInt64 lastUploadedPiece, downloadingFilePiecesNumber;
	bool enableTimeout = false;

	std::vector<std::string> uploadedFiles;
	std::string uploadingFileName;
	std::ofstream uploadingFile;
	std::ifstream downloadingFile;

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
		/*char ch;
		iResult = socket->ReceiveOOBByte(ch);
		if (iResult == 1)
			std::cout << "Hustone, we have liftoff.\n";*/

		iResult = socket->Receive(packet, enableTimeout ? 5 : -1);
		if (iResult == SOCKET_ERROR || (enableTimeout && iResult == 0))
		{
			socket->Close();
			continue;
		}

		packet >> packetType;
		switch (packetType)
		{
			case HANDSHAKE:
			{
				enableTimeout = false;
				break;
			}
			case ECHO:
			{
				std::string msg;
				packet >> msg;
				std::cout << "Echo command.\nClient says: " << msg << std::endl;
				packet.Clear();
				packet << ECHO << msg;
				socket->Send(packet);

				char ch = 'a';
				sleep(100);
				socket->SendOOBByte(ch);

				break;

			}
			case TIME:
			{
				char buff[20];
				time_t now = time(nullptr);

				strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

				std::cout << "Time command.\nCurrent time " << buff << std::endl;

				packet.Clear();
				packet << TIME << buff;

				socket->Send(packet);

				break;
			}
			case CLOSE_CONNECTION:
			{
				std::cout << "Close command.\nClosing current connection.\n";
				socket->Close();
				break;
			}
			case UPLOAD_PIECE:
			{
				UInt32 size;
				UInt64 pieceNumber;
				
				packet >> pieceNumber >> size;

				char* piece = new char[size];
				packet.ExtractRawData(piece);

				uploadingFile.seekp(pieceNumber * DEFAULT_BUFFLEN);
				uploadingFile.write(piece, size);
				delete piece;

				lastUploadedPiece = pieceNumber;

				break;
			}
			case CONTINUE_INTERRUPTED_UPLOAD:
			{
				packet.Clear();
				packet << LAST_RECEIVED_PIECE << lastUploadedPiece;
				socket->Send(packet);
				break;
			}
			case FILE_UPLOAD_COMPLETE:
			{
				uploadedFiles.push_back(uploadingFileName);
				uploadingFile.close();
				uploadingFileName.clear();

				enableTimeout = false;
				break;
			}
			case DOWNLOAD_PIECE:
			{
				bool downloadStopped = false;
				UInt64 pieceNumber, fileSize, piecesNumber;
				char* piece = new char[DEFAULT_BUFFLEN];

				packet >> pieceNumber;

				downloadingFile.seekg(0, std::ios_base::end);
				fileSize = downloadingFile.tellg();

				piecesNumber = fileSize / DEFAULT_BUFFLEN;
				piecesNumber += (fileSize % DEFAULT_BUFFLEN) ? 1 : 0;

				for (UInt64 i = pieceNumber; i < piecesNumber; i++)
				{
					UInt32 bytesToRead = (i == piecesNumber - 1) ? fileSize % DEFAULT_BUFFLEN : DEFAULT_BUFFLEN;

					downloadingFile.seekg(i * DEFAULT_BUFFLEN);
					downloadingFile.read(piece, bytesToRead);

					packet.Clear();
					packet << DOWNLOAD_PIECE << i << bytesToRead;
					packet.AddRawData(piece, bytesToRead);

					iResult = socket->Send(packet, 5);
					if (iResult <= 0)
					{
						downloadStopped = true;
						break;
					}
				}

				if (downloadStopped)
					break;

				packet.Clear();
				packet << FILE_DOWLOAD_COMPLETE;
				socket->Send(packet);

				break;
			}
			case SHUTDOWN:
			{
				std::cout << "\nShutdown command.\nShuting down.\n";
				listener.Close();
				socket->Close();
				SocketCleanUp();

				return 0;
			}
			case UPLOAD_REQUEST:
			{
				if (uploadingFile.is_open())
				{
					uploadingFile.close();
					std::remove(uploadingFileName.c_str());
				}

				packet >> uploadingFileName;
				
				uploadingFile.open(uploadingFileName, std::ios::binary);

				enableTimeout = true;

				break;
			}
			case DOWNLOAD_REQUEST:
			{
				if (downloadingFile.is_open())
					downloadingFile.close();

				std::string fileName;
				UInt8 code;
				
				packet >> fileName;

				std::vector<std::string>::iterator iter = std::find(uploadedFiles.begin(), uploadedFiles.end(), fileName);
				if (iter == uploadedFiles.end())
				{
					code = NO_SUCH_FILE;
				}
				else
				{
					downloadingFile.open(uploadedFiles[iter - uploadedFiles.begin()], std::ios::binary);
					code = DOWNLOAD_CONFIRM;
				}

				packet.Clear();
				packet << code;
				socket->Send(packet);
				
				break;
			}
			default:
			{
				std::cout << packetType << " unknown command./n";
			}
		}
	}
}