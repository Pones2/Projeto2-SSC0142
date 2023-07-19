run_client: client
	./client

run_server: server
	./server

client: client.cpp ClientSocket.cpp ClientSocket.hpp 
	g++ client.cpp ClientSocket.cpp -o client

server: server.cpp ServerSocket.cpp ServerSocket.hpp
	g++ server.cpp ServerSocket.cpp -o server

clean:
	rm *.o
