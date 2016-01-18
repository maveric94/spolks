all: server client

OBJS = unix/obj/client.o  \
       unix/obj/server.o \
       unix/obj/Packet.o  \
       unix/obj/util.o  \
       unix/obj/TCPListener.o  \
	   unix/obj/TCPSocket.o \
	   unix/obj/UDPSocket.o 

SERVER_OBJS = unix/obj/server.o \
       unix/obj/Packet.o  \
       unix/obj/util.o  \
       unix/obj/TCPListener.o  \
	   unix/obj/TCPSocket.o \
	   unix/obj/UDPSocket.o 

CLIENT_OBJS = unix/obj/client.o  \
       unix/obj/Packet.o  \
       unix/obj/util.o  \
	   unix/obj/TCPSocket.o \
	   unix/obj/UDPSocket.o 

CPPFLAGS = -Isource

clean:
	$(RM) -rf $(OBJS) unix/bin/server unix/bin/client

rebuild: clean all

unix/obj/%.o: source/%.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

server: $(SERVER_OBJS)
	g++ -o unix/bin/$@ $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	g++ -o unix/bin/$@ $(CLIENT_OBJS) 