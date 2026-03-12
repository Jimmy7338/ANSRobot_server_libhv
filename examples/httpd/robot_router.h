#ifndef ROBOT_ROUTER_H
#define ROBOT_ROUTER_H

#include "HttpService.h"

class RobotRouter {
public:
    static void Register(hv::HttpService& router);
};

#endif // ROBOT_ROUTER_H
