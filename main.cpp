#include <iostream>
#include "net.h"


int main()
{


    CServer server;
    server.AddTcp(6925);
    server.Start();


    return 0;
}