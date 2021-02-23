#include "./WebServer/WebServer.h"

int main(int argc, char *argv[])
{
    WebServer server;
    server.init(9006, 4, 1);

    server.eventListen();

    server.eventLoop();

    return 0;
}
