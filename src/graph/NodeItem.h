#pragma once

#include <QGraphicsItem>
#include <QString>
#include <QVector>
#include <QColor>
#include <QPainter>

class ConnectionLine;
class QGraphicsProxyWidget;
class QWidget;

enum class PinType {
    Any = 0,
    Text,
    Image,
    Style,
    Json
};

struct PinInfo
{
    QString name;
    bool    isOutput    = false;
    bool    multiInput  = false;
    PinType type        = PinType::Any;
};

class NodeItem : public QGraphicsItem
{
public:
    explicit NodeItem(const QString &title,
                      const QVector<PinInfo> &pins,
                      const QColor &headerColor = QColor(60, 120, 200),
                      QGraphicsItem *parent = nullptr);
    ~NodeItem() override = default;

    QRectF boundingRect() const override;
    void   paint(QPainter *painter,
                 const QStyleOptionGraphicsItem *option,
                 QWidget *widget) override;

    int        pinCount()                          const { return m_pins.size(); }
    PinInfo    pinAt(int index)                    const { return m_pins.at(index); }
    QPointF    pinScenePos(int index)              const;
    int        pinIndexAt(const QPointF &scenePos) const;

    void addConnection(ConnectionLine *line);
    void removeConnection(ConnectionLine *line);
    const QVector<ConnectionLine *> &connections() const { return m_connections; }

    bool inputPinHasConnection(int pinIndex) const;
    int  inputPinConnectionCount(int pinIndex) const;

    QString title() const { return m_title; }

    QColor headerColor() const { return m_headerColor; }
    void   setHeaderColor(const QColor &color);

    qreal currentWidth()  const { return m_width; }
    qreal currentHeight() const { return nodeHeight(); }
    qreal currentWidgetHeight() const { return m_widgetHeight; }

    void resizeNode(qreal width, qreal widgetHeight);

protected:
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant &value) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void setEmbeddedWidget(QWidget *widget);
    void updateEmbeddedSize(qreal newHeight);

    static constexpr qreal DefaultWidth = 260.0;
    static constexpr qreal MinWidth     = 180.0;
    static constexpr qreal MinHeight    = 100.0;
    static constexpr qreal HeaderH      = 30.0;
    static constexpr qreal PinSpacing   = 26.0;
    static constexpr qreal PinRadius    =  6.0;
    static constexpr qreal PinMargin    = 12.0;
    static constexpr qreal CornerR      =  8.0;
    static constexpr qreal WidgetPad    =  8.0;
    static constexpr qreal ResizeGrip   = 14.0;

    qreal pinsHeight() const;
    qreal nodeHeight() const;
    QPointF pinLocalPos(int index) const;

    QString               m_title;
    QVector<PinInfo>      m_pins;
    QVector<ConnectionLine *> m_connections;
    QColor                m_headerColor;

    QGraphicsProxyWidget *m_proxy = nullptr;
    QWidget              *m_embeddedWidget = nullptr;
    qreal                 m_widgetHeight = 0.0;
    qreal                 m_width = DefaultWidth;

private:
    bool isInResizeGrip(const QPointF &localPos) const;

    bool    m_resizing = false;
    QPointF m_resizeOrigin;
    qreal   m_resizeOrigW = 0;
    qreal   m_resizeOrigH = 0;
};
