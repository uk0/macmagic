#import "wifiinfo.h"
#import <CoreWLAN/CoreWLAN.h>
#import <Foundation/Foundation.h>

static QString securityName(CWSecurity sec) {
    switch (sec) {
        case kCWSecurityNone:           return QStringLiteral("Open");
        case kCWSecurityWEP:            return QStringLiteral("WEP");
        case kCWSecurityWPAPersonal:    return QStringLiteral("WPA Personal");
        case kCWSecurityWPA2Personal:   return QStringLiteral("WPA2 Personal");
        case kCWSecurityWPA3Personal:   return QStringLiteral("WPA3 Personal");
        case kCWSecurityWPAEnterprise:  return QStringLiteral("WPA Enterprise");
        case kCWSecurityWPA2Enterprise: return QStringLiteral("WPA2 Enterprise");
        case kCWSecurityWPA3Enterprise: return QStringLiteral("WPA3 Enterprise");
        default:                        return QStringLiteral("Unknown");
    }
}

static QString phyModeName(CWPHYMode mode) {
    switch (mode) {
        case kCWPHYMode11a:  return QStringLiteral("802.11a");
        case kCWPHYMode11b:  return QStringLiteral("802.11b");
        case kCWPHYMode11g:  return QStringLiteral("802.11g");
        case kCWPHYMode11n:  return QStringLiteral("802.11n");
        case kCWPHYMode11ac: return QStringLiteral("802.11ac");
        case (CWPHYMode)6:   return QStringLiteral("802.11ax");
        default:             return QStringLiteral("Unknown");
    }
}

WifiDetails getWifiDetails(const QString &device)
{
    WifiDetails info;

    @autoreleasepool {
        CWWiFiClient *client = [CWWiFiClient sharedWiFiClient];
        NSString *devName = device.toNSString();
        CWInterface *iface = [client interfaceWithName:devName];
        if (!iface)
            iface = [client interface];
        if (!iface)
            return info;

        // Connected if power is on and RSSI is non-zero
        info.connected = iface.powerOn && iface.rssiValue != 0;
        if (!info.connected)
            return info;

        // SSID may be nil due to macOS privacy (needs Location Services)
        if (iface.ssid)
            info.ssid = QString::fromNSString(iface.ssid);

        info.rssi    = (int)iface.rssiValue;
        info.noise   = (int)iface.noiseMeasurement;
        info.channel = (int)iface.wlanChannel.channelNumber;
        info.txRate  = iface.transmitRate;

        if (iface.wlanChannel.channelBand == kCWChannelBand5GHz)
            info.band = QStringLiteral("5GHz");
        else
            info.band = QStringLiteral("2.4GHz");

        info.security = securityName(iface.security);
        info.phyMode  = phyModeName(iface.activePHYMode);
    }

    return info;
}
