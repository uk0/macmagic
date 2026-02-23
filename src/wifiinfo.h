#pragma once
#include <QString>

struct WifiDetails {
    bool connected = false;
    QString ssid;           // may be empty due to macOS privacy
    int rssi = 0;           // dBm
    int noise = 0;          // dBm
    int channel = 0;
    QString band;           // "2.4GHz" or "5GHz"
    double txRate = 0;      // Mbps
    QString security;       // "WPA2 Personal", "WPA3 Personal", etc.
    QString phyMode;        // "802.11ax", "802.11ac", etc.
};

// CoreWLAN-based Wi-Fi info (Objective-C++ implementation)
WifiDetails getWifiDetails(const QString &device);
