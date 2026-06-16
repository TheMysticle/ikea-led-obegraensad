#pragma once

class ConnectionManager {
public:
    static void init();
    static void connectToWiFi();
    static void checkWiFiConnection();
};
