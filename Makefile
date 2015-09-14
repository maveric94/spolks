all: server client

OBJS = unix/obj/client.o  \
       unix/obj/server.o \
       unix/obj/Packet.o  \
       unix/obj/Socket.o  \
       unix/obj/TCPListener.o  \
	   unix/obj/TCPSocket.o 

SERVER_OBJS = unix/obj//server.o \
       unix/obj/Packet.o  \
       unix/obj/Socket.o  \
       unix/obj/TCPListener.o  \
	   unix/obj/TCPSocket.o 

CLIENT_OBJS = unix/obj/client.o  \
       unix/obj/Packet.o  \
       unix/obj/Socket.o  \
	   unix/obj/TCPSocket.o 

CPPFLAGS = -std=c++11 -Isource

clean:
	$(RM) -rf $(OBJS) server client

rebuild: clean all

unix/obj/%.o: source/%.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

server: $(SERVER_OBJS)
	g++ -o unix/bin/$@ $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	g++ -o unix/bin/$@ $(CLIENT_OBJS) 