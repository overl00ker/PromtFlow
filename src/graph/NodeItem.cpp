#include "NodeItem.h"
#include "ConnectionLine.h"

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QFontMetrics>
#include <QPen>
#include <QWidget>
#include <QCursor>
#include <QApplication>

NodeItem::NodeItem(const QString &title,
                   const QVector<PinInfo> &pins,
                   const QColor &headerColor,
                   QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_title(title)
    , m_pins(pins)
    , m_headerColor(headerColor)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCacheMode(DeviceCoordinateCache);
}

void NodeItem::setHeaderColor(const QColor &color)
{
    m_headerColor = color;
    update();
}

void NodeItem::setEmbeddedWidget(QWidget *widget)
{
    m_embeddedWidget = widget;
    widget->setFixedWidth(static_cast<int>(m_width - 2 * WidgetPad));

    widget->setStyleSheet(
        "QWidget { background: #2d2d30; color: #ddd; border: none; font-size: 12px; }"
        "QPlainTextEdit { background: #1e1e1e; color: #ccc; border: 1px solid #555; border-radius: 3px; padding: 4px; }"
        "QTextEdit { background: #1e1e1e; color: #ccc; border: 1px solid #555; border-radius: 3px; padding: 4px; }"
        "QLabel { background: transparent; color: #bbb; }"
        "QPushButton { background: #3c3c3c; color: #ddd; border: 1px solid #555; border-radius: 3px; padding: 4px 12px; }"
        "QPushButton:hover { background: #505050; }"
        "QComboBox { background: #1e1e1e; color: #ccc; border: 1px solid #555; border-radius: 3px; padding: 3px 8px; }"
        "QComboBox QAbstractItemView { background: #2d2d30; color: #ccc; selection-background-color: #3c78b5; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right;"
        "  width: 18px; border-left: 1px solid #555; }"
        "QComboBox::down-arrow { width: 0; height: 0;"
        "  border-left: 4px solid transparent; border-right: 4px solid transparent;"
        "  border-top: 5px solid #aaa; }"
        "QLineEdit { background: #1e1e1e; color: #ccc; border: 1px solid #555; border-radius: 3px; padding: 3px; }"
    );

    m_widgetHeight = widget->sizeHint().height();

    m_proxy = new QGraphicsProxyWidget(this);
    m_proxy->setWidget(widget);
    
    resizeNode(m_width, m_widgetHeight);

    m_proxy->setFlag(ItemIsMovable, false);
    m_proxy->setAcceptedMouseButtons(Qt::AllButtons);

    setCacheMode(NoCache);
    prepareGeometryChange();
}

void NodeItem::updateEmbeddedSize(qreal newHeight)
{
    if (!m_embeddedWidget) return;

    m_widgetHeight = newHeight;
    m_embeddedWidget->setFixedHeight(static_cast<int>(newHeight));

    prepareGeometryChange();

    if (m_proxy)
        m_proxy->setPos(WidgetPad, HeaderH + pinsHeight());

    for (auto *line : m_connections)
        line->updatePath();

    update();
}

qreal NodeItem::pinsHeight() const
{
    int inputs = 0, outputs = 0;
    for (const auto &p : m_pins)
        p.isOutput ? ++outputs : ++inputs;
    int maxRows = qMax(inputs, outputs);
    return PinSpacing * qMax(maxRows, 0) + (maxRows > 0 ? PinSpacing * 0.5 : 4.0);
}

qreal NodeItem::nodeHeight() const
{
    qreal h = HeaderH + pinsHeight();
    if (m_embeddedWidget)
        h += m_widgetHeight + WidgetPad * 2;
    return h;
}

QRectF NodeItem::boundingRect() const
{

    return QRectF(0, 0, m_width + 4, nodeHeight() + 4);
}

void NodeItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget *)
{
    const QRectF rect(0, 0, m_width, nodeHeight());
    const bool selected = (option->state & QStyle::State_Selected);

    painter->setRenderHint(QPainter::Antialiasing);


    {
        QRectF shadowRect = rect.adjusted(3, 3, 3, 3);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 60));
        painter->drawRoundedRect(shadowRect, CornerR, CornerR);
    }


    QPainterPath bodyPath;
    bodyPath.addRoundedRect(rect, CornerR, CornerR);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(45, 45, 48));
    painter->drawPath(bodyPath);


    {
        QLinearGradient grad(0, 0, m_width, 0);
        grad.setColorAt(0.0, m_headerColor);
        grad.setColorAt(1.0, m_headerColor.darker(130));

        QPainterPath headerPath;
        headerPath.addRoundedRect(QRectF(0, 0, m_width, HeaderH), CornerR, CornerR);
        headerPath.addRect(QRectF(0, HeaderH - CornerR, m_width, CornerR));
        painter->setBrush(grad);
        painter->drawPath(headerPath.simplified());
    }


    painter->setPen(Qt::white);
    QFont font;
    font.setFamily("Segoe UI");
    font.setBold(true);
    font.setPixelSize(13);
    painter->setFont(font);
    painter->drawText(QRectF(PinMargin, 0, m_width - 2 * PinMargin, HeaderH),
                      Qt::AlignVCenter | Qt::AlignLeft, m_title);


    QFont pinFont;
    pinFont.setFamily("Segoe UI");
    pinFont.setPixelSize(11);
    painter->setFont(pinFont);

    for (int i = 0; i < m_pins.size(); ++i) {
        const QPointF center = pinLocalPos(i);
        const auto &pin = m_pins[i];

        QColor pinColor;
        if (pin.isOutput)
            pinColor = QColor(100, 220, 100);
        else if (pin.multiInput)
            pinColor = QColor(100, 200, 220);
        else
            pinColor = QColor(220, 160, 60);

        painter->setPen(QPen(Qt::white, 1.5));
        painter->setBrush(pinColor);
        painter->drawEllipse(center, PinRadius, PinRadius);

        if (pin.multiInput && !pin.isOutput) {
            painter->setPen(QPen(Qt::white, 1.5));
            painter->drawLine(center + QPointF(-3, 0), center + QPointF(3, 0));
            painter->drawLine(center + QPointF(0, -3), center + QPointF(0, 3));
        }

        painter->setPen(QColor(200, 200, 200));
        if (pin.isOutput) {
            QRectF labelRect(center.x() - m_width + PinMargin + PinRadius * 2,
                             center.y() - PinSpacing / 2,
                             m_width - PinMargin * 2 - PinRadius * 2,
                             PinSpacing);
            painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignRight, pin.name);
        } else {
            QRectF labelRect(center.x() + PinRadius + 4,
                             center.y() - PinSpacing / 2,
                             m_width - PinMargin * 2 - PinRadius * 2,
                             PinSpacing);
            painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, pin.name);
        }
    }


    {
        qreal nh = nodeHeight();
        QPointF p1(m_width, nh);
        QPointF p2(m_width - ResizeGrip, nh);
        QPointF p3(m_width, nh - ResizeGrip);

        QPainterPath gripPath;
        gripPath.moveTo(p1);
        gripPath.lineTo(p2);
        gripPath.lineTo(p3);
        gripPath.closeSubpath();

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 255, selected ? 60 : 30));
        painter->drawPath(gripPath);


        painter->setPen(QPen(QColor(255, 255, 255, selected ? 100 : 50), 1));
        painter->drawLine(QPointF(m_width - 4, nh - 1),
                          QPointF(m_width - 1, nh - 4));
        painter->drawLine(QPointF(m_width - 8, nh - 1),
                          QPointF(m_width - 1, nh - 8));
    }


    if (selected) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(255, 200, 50), 2));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), CornerR, CornerR);
    }
}

QPointF NodeItem::pinLocalPos(int index) const
{
    const auto &pin = m_pins.at(index);

    int order = 0;
    for (int i = 0; i < index; ++i)
        if (m_pins[i].isOutput == pin.isOutput)
            ++order;

    qreal x = pin.isOutput ? (m_width - PinMargin) : PinMargin;
    qreal y = HeaderH + PinSpacing * (order + 0.5);
    return QPointF(x, y);
}

QPointF NodeItem::pinScenePos(int index) const
{
    return mapToScene(pinLocalPos(index));
}

int NodeItem::pinIndexAt(const QPointF &scenePos) const
{
    QPointF local = mapFromScene(scenePos);
    for (int i = 0; i < m_pins.size(); ++i) {
        if (QLineF(local, pinLocalPos(i)).length() <= PinRadius + 4)
            return i;
    }
    return -1;
}

void NodeItem::addConnection(ConnectionLine *line)
{
    if (!m_connections.contains(line))
        m_connections.append(line);
}

void NodeItem::removeConnection(ConnectionLine *line)
{
    m_connections.removeAll(line);
}

bool NodeItem::inputPinHasConnection(int pinIndex) const
{
    return inputPinConnectionCount(pinIndex) > 0;
}

int NodeItem::inputPinConnectionCount(int pinIndex) const
{
    int count = 0;
    for (auto *line : m_connections) {
        if (line->targetNode() == this && line->targetPin() == pinIndex)
            ++count;
    }
    return count;
}

QVariant NodeItem::itemChange(GraphicsItemChange change,
                              const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        for (auto *line : m_connections)
            line->updatePath();
    }
    return QGraphicsItem::itemChange(change, value);
}



bool NodeItem::isInResizeGrip(const QPointF &localPos) const
{
    qreal nh = nodeHeight();

    qreal dx = m_width - localPos.x();
    qreal dy = nh - localPos.y();
    return (dx >= 0 && dy >= 0 && dx <= ResizeGrip && dy <= ResizeGrip && (dx + dy) <= ResizeGrip);
}

void NodeItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isInResizeGrip(event->pos())) {
        setCursor(Qt::SizeFDiagCursor);
    } else {
        unsetCursor();
    }
    QGraphicsItem::hoverMoveEvent(event);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isInResizeGrip(event->pos())) {
        m_resizing = true;
        m_resizeOrigin = event->scenePos();
        m_resizeOrigW = m_width;
        m_resizeOrigH = m_widgetHeight;
        event->accept();
        return;
    }
    QGraphicsItem::mousePressEvent(event);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizing) {
        QPointF delta = event->scenePos() - m_resizeOrigin;

        qreal newW = qMax(MinWidth, m_resizeOrigW + delta.x());
        qreal newWidgetH = qMax(40.0, m_resizeOrigH + delta.y());

        resizeNode(newW, newWidgetH);

        event->accept();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizing) {
        m_resizing = false;
        event->accept();
        return;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void NodeItem::resizeNode(qreal width, qreal widgetHeight)
{
    prepareGeometryChange();
    m_width = qMax(MinWidth, width);
    qreal newWidgetH = qMax(40.0, widgetHeight);

    if (m_embeddedWidget) {
        m_embeddedWidget->setFixedWidth(static_cast<int>(m_width - 2 * WidgetPad));
        m_embeddedWidget->setFixedHeight(static_cast<int>(newWidgetH));
        m_widgetHeight = newWidgetH;
    }

    if (m_proxy)
        m_proxy->setPos(WidgetPad, HeaderH + pinsHeight());

    for (auto *line : m_connections)
        line->updatePath();

    update();
}

