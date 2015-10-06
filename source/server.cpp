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
	UInt32 lastUploadedPiece;
	bool enableDelay = false;

	std::vector<std::string> uploadedFiles;
	std::string uploadingFileName;
	std::ofstream uploadingFile;

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

		iResult = socket->Receive(packet, enableDelay ? 5 : -1);
		if (iResult == SOCKET_ERROR || (enableDelay && iResult == 0))
		{
			socket->Close();
			continue;
		}

		packet >> packetType;
		switch (packetType)
		{
			case ECHO:
			{
				std::string msg;
				packet >> msg;
				std::cout << "Echo command.\nClient says: " << msg << std::endl;
				packet.Clear();
				packet << ECHO << msg;
				socket->Send(packet);
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
				UInt32 pieceNumber;
				
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

				enableDelay = false;
				break;
			}
			case DOWNLOAD_PIECE:
			{
				
				
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

				enableDelay = true;

				break;
			}
			case GET_FILE_ID:
			{
				std::string fileName;
				Int32 fileID;
				UInt64 fileSize;
				
				packet >> fileName;

				std::vector<std::string>::iterator iter = std::find(uploadedFiles.begin(), uploadedFiles.end(), fileName);
				if (iter == uploadedFiles.end())
				{
					fileID = -1;
					fileSize = -1;
				}
				else
				{
					fileID = iter - uploadedFiles.begin();

					std::ifstream file(uploadedFiles[fileID], std::ios::binary | std::ios::ate);
					fileSize = file.tellg();
					file.close();
				}

				packet.Clear();
				packet << FILE_ID << fileID << fileSize;

				iResult = socket->Send(packet);
				break;
			}
			default:
			{
				std::cout << packetType << " unknown command./n";
			}
		}
	}
}