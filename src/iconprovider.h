#pragma once
#include <QIcon>

class IconProvider {
public:
    // Tray icons (44x44 px, ratio=2.0, template images)
    static QIcon trayNormal();
    static QIcon trayActive();

    // App icon (256x256 px, colorful)
    static QIcon appIcon();

    // Menu item icons (32x32 px, ratio=2.0)
    static QIcon greenDot();
    static QIcon grayDot();
    static QIcon diceIcon();
    static QIcon pencilIcon();
    static QIcon stopIcon();
    static QIcon refreshIcon();
    static QIcon infoIcon();
    static QIcon quitIcon();
    static QIcon networkIcon();
    static QIcon shieldIcon();
    static QIcon wifiIcon();
    static QIcon warningIcon();

private:
    static QPixmap createMenuPixmap();
    static void drawShield(QPainter &p, const QRectF &rect, bool filled);
    static void drawSignalArcs(QPainter &p, const QPointF &center, qreal radius, int count);
};
