CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ../code/http/*.cpp ../code/buffer/*.cpp ../code/main.cpp ../code/WebServer.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread

clean:
	rm -rf ../bin/* $(TARGET)