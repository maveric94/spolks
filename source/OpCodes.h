#ifndef OPCODES_H
#define OPCODES_H

#include "TypeDefs.h"

const static UInt8 ECHO = 0x01;
const static UInt8 TIME = 0x02;
const static UInt8 CLOSE_CONNECTION = 0x03;
const static UInt8 DOWNLOAD_PIECE = 0x04;
const static UInt8 UPLOAD_PIECE = 0x05;
const static UInt8 SHUTDOWN = 0x06;
const static UInt8 UPLOAD_REQUEST = 0x07;

const static UInt8 GET_FILES = 0x09;
const static UInt8 FILE_UPLOAD_COMPLETE = 0x0A;
const static UInt8 GET_FILE_ID = 0x0B;
const static UInt8 FILE_ID = 0x0C;
const static UInt8 CONTINUE_INTERRUPTED_UPLOAD = 0x0D;
const static UInt8 LAST_RECEIVED_PIECE = 0x0E;

const static UInt32 DEFAULT_BUFFLEN = 2048;
#define DEFAULT_PORT "27020"

#endif