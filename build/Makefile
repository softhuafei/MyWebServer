CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = ../code/buffer/*.cpp \
		../code/http/*.cpp  \
		../code/threadpool/*.cpp \
		../code/util/*.cpp \
		../code/WebServer/*.cpp \
		../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread

clean:
	rm -rf ../bin/* $(TARGET)