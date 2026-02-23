#include "iconprovider.h"

#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// 32x32 physical pixels, ratio 2.0 → 16x16 logical points.
QPixmap IconProvider::createMenuPixmap()
{
    QPixmap pm(32, 32);
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);
    return pm;
}

// Create a menu icon that adapts to light/dark mode automatically.
// Draw in black on transparent → setIsMask(true) → macOS tints it.
static QIcon makeMenuMaskIcon(const QPixmap &pm)
{
    QIcon icon(pm);
    icon.setIsMask(true);
    return icon;
}

void IconProvider::drawShield(QPainter &p, const QRectF &rect, bool filled)
{
    QPainterPath path;
    qreal x = rect.x();
    qreal y = rect.y();
    qreal w = rect.width();
    qreal h = rect.height();
    qreal midX = x + w / 2.0;
    qreal r = w * 0.22;

    path.moveTo(x, y + r);
    path.quadTo(x, y, x + r, y);
    path.lineTo(x + w - r, y);
    path.quadTo(x + w, y, x + w, y + r);
    path.lineTo(x + w, y + h * 0.52);
    path.quadTo(x + w, y + h * 0.62, midX, y + h);
    path.quadTo(x, y + h * 0.62, x, y + h * 0.52);
    path.closeSubpath();

    if (filled)
        p.fillPath(path, p.brush());
    p.drawPath(path);
}

void IconProvider::drawSignalArcs(QPainter &p, const QPointF &center, qreal radius, int count)
{
    int startAngle = 30 * 16;
    int spanAngle  = 120 * 16;

    for (int i = 0; i < count; ++i) {
        qreal r = radius * (1.0 + i * 0.65);
        QRectF arcRect(center.x() - r, center.y() - r, r * 2, r * 2);
        p.drawArc(arcRect, startAngle, spanAngle);
    }
}

// ---------------------------------------------------------------------------
// Tray Icons — 44x44 px, ratio 2.0 → 22x22 logical points
// setIsMask(true) → macOS auto-tints for light/dark menu bar
// ---------------------------------------------------------------------------

QIcon IconProvider::trayNormal()
{
    QPixmap pm(44, 44);
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.4));
    p.setBrush(Qt::NoBrush);
    drawShield(p, QRectF(5, 2, 12, 16), false);

    p.end();

    QIcon icon(pm);
    icon.setIsMask(true);
    return icon;
}

QIcon IconProvider::trayActive()
{
    QPixmap pm(44, 44);
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.4));
    p.setBrush(Qt::black);
    drawShield(p, QRectF(5, 2, 12, 16), true);

    p.end();

    QIcon icon(pm);
    icon.setIsMask(true);
    return icon;
}

// ---------------------------------------------------------------------------
// App Icon — 256x256 px (used for dialogs, About box)
// ---------------------------------------------------------------------------

QIcon IconProvider::appIcon()
{
    QPixmap pm(256, 256);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Rounded rect background with macOS 26 style gradient
    QLinearGradient grad(0, 0, 256, 256);
    grad.setColorAt(0.0, QColor(0x00, 0x71, 0xe3));  // system blue
    grad.setColorAt(1.0, QColor(0x5a, 0xc8, 0xfa));  // lighter blue
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRoundedRect(QRectF(0, 0, 256, 256), 48, 48);

    // White shield
    p.setPen(QPen(Qt::white, 3.0));
    p.setBrush(Qt::white);
    drawShield(p, QRectF(45, 40, 125, 170), true);

    // Cyan signal arcs — macOS 26 teal
    p.setPen(QPen(QColor(0x64, 0xd2, 0xff), 6.0));
    p.setBrush(Qt::NoBrush);
    drawSignalArcs(p, QPointF(175, 50), 14.0, 3);

    p.end();
    return QIcon(pm);
}

// ---------------------------------------------------------------------------
// Menu Icons — 32x32 px, ratio 2.0 → 16x16 logical points
// Monochrome icons use setIsMask(true) for automatic dark/light mode.
// Colored icons (dots, stop, warning) keep fixed colors.
// ---------------------------------------------------------------------------

QIcon IconProvider::greenDot()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x30, 0xd1, 0x58));  // macOS 26 system green
    p.drawEllipse(QPointF(8, 8), 4.0, 4.0);
    p.end();
    return QIcon(pm);  // colored — no mask
}

QIcon IconProvider::grayDot()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xae, 0xae, 0xb2));  // macOS 26 system gray
    p.drawEllipse(QPointF(8, 8), 4.0, 4.0);
    p.end();
    return QIcon(pm);  // colored — no mask
}

QIcon IconProvider::diceIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Rounded rect body
    p.setPen(QPen(Qt::black, 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2, 2, 12, 12), 2.5, 2.5);

    // Five dots (dice face 5)
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    qreal dr = 1.2;
    p.drawEllipse(QPointF(5.2, 5.2), dr, dr);
    p.drawEllipse(QPointF(10.8, 5.2), dr, dr);
    p.drawEllipse(QPointF(8, 8), dr, dr);
    p.drawEllipse(QPointF(5.2, 10.8), dr, dr);
    p.drawEllipse(QPointF(10.8, 10.8), dr, dr);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::pencilIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.1));
    p.setBrush(Qt::NoBrush);

    // Pencil body — diagonal parallelogram
    QPainterPath body;
    body.moveTo(11.5, 2);
    body.lineTo(14, 4.5);
    body.lineTo(5.5, 13);
    body.lineTo(3, 10.5);
    body.closeSubpath();
    p.drawPath(body);

    // Collar line
    p.drawLine(QPointF(4.5, 11.5), QPointF(7, 11.5));

    // Filled tip
    QPainterPath tip;
    tip.moveTo(3, 10.5);
    tip.lineTo(5.5, 13);
    tip.lineTo(2, 14);
    tip.closeSubpath();
    p.setBrush(Qt::black);
    p.drawPath(tip);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::stopIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xff, 0x45, 0x3a));  // macOS 26 system red
    p.drawRoundedRect(QRectF(3.5, 3.5, 9, 9), 1.5, 1.5);
    p.end();
    return QIcon(pm);  // colored — no mask
}

QIcon IconProvider::refreshIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.3));
    p.setBrush(Qt::NoBrush);

    // Circular arc ~300°
    p.drawArc(QRectF(2.5, 2.5, 11, 11), 40 * 16, 300 * 16);

    // Arrowhead at arc end (top-right)
    QPainterPath arrow;
    arrow.moveTo(11.5, 5);
    arrow.lineTo(14, 2.5);
    arrow.lineTo(11, 2);
    p.setPen(QPen(Qt::black, 1.0));
    p.drawPath(arrow);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::infoIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(8, 8), 5.5, 5.5);

    // "i" letter
    QFont font;
    font.setPixelSize(9);
    font.setBold(true);
    font.setFamily(QStringLiteral("Helvetica"));
    p.setFont(font);
    p.setPen(Qt::black);
    p.drawText(QRectF(2.5, 2.5, 11, 11), Qt::AlignCenter, QStringLiteral("i"));

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::quitIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.3));
    p.setBrush(Qt::NoBrush);

    // Power symbol: circle with 60° gap centered at top (90°)
    // Arc from 120° CCW 300° → ends at 60°, gap is 60°–120° centered at 90°
    p.drawArc(QRectF(3, 3, 10, 10), 120 * 16, 300 * 16);
    // Vertical stem through the gap
    p.drawLine(QPointF(8, 2), QPointF(8, 7));

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::networkIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 0.8));

    // Three nodes in a triangle with connecting lines
    QPointF top(8, 3.5);
    QPointF bl(3.5, 12.5);
    QPointF br(12.5, 12.5);

    p.drawLine(top, bl);
    p.drawLine(top, br);
    p.drawLine(bl, br);

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawEllipse(top, 2.0, 2.0);
    p.drawEllipse(bl, 2.0, 2.0);
    p.drawEllipse(br, 2.0, 2.0);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::shieldIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1.1));
    p.setBrush(Qt::NoBrush);
    drawShield(p, QRectF(3, 1.5, 10, 13), false);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::wifiIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Standard wifi: arcs radiating upward from bottom center
    QPointF base(8, 13);
    p.setPen(QPen(Qt::black, 1.1));
    p.setBrush(Qt::NoBrush);

    // Three concentric arcs (45° to 135° = upper half)
    for (int i = 0; i < 3; ++i) {
        qreal r = 3.0 + i * 2.5;
        QRectF arcRect(base.x() - r, base.y() - r, r * 2, r * 2);
        p.drawArc(arcRect, 45 * 16, 90 * 16);
    }

    // Dot at base
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawEllipse(base, 1.3, 1.3);

    p.end();
    return makeMenuMaskIcon(pm);
}

QIcon IconProvider::warningIcon()
{
    QPixmap pm = createMenuPixmap();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QColor orange(0xff, 0x9f, 0x0a);  // macOS 26 system orange
    p.setPen(QPen(orange, 1.2));
    p.setBrush(Qt::NoBrush);

    // Triangle
    QPainterPath tri;
    tri.moveTo(8, 2);
    tri.lineTo(14.5, 13.5);
    tri.lineTo(1.5, 13.5);
    tri.closeSubpath();
    p.drawPath(tri);

    // "!" exclamation
    QFont font;
    font.setPixelSize(8);
    font.setBold(true);
    font.setFamily(QStringLiteral("Helvetica"));
    p.setFont(font);
    p.setPen(orange);
    p.drawText(QRectF(2, 4, 12, 10), Qt::AlignCenter, QStringLiteral("!"));

    p.end();
    return QIcon(pm);  // colored — no mask
}
