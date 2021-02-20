#include "WebServer.h"

int main(int argc, char *argv[])
{
    WebServer server;
    server.init(9056, 8, 1);

    server.eventListen();

    server.eventLoop();

    return 0;
}
