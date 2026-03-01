#include "ConnectionLine.h"
#include "NodeItem.h"

#include <QPen>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

ConnectionLine::ConnectionLine(QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
{
    setPen(QPen(QColor(180, 180, 180), 3.0, Qt::SolidLine, Qt::RoundCap));
    setZValue(-1);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void ConnectionLine::setSource(NodeItem *node, int pinIndex)
{
    m_srcNode = node;
    m_srcPin  = pinIndex;
    m_dragging = false;
    if (node) node->addConnection(this);
    updatePath();
}

void ConnectionLine::setTarget(NodeItem *node, int pinIndex)
{
    m_dstNode = node;
    m_dstPin  = pinIndex;
    m_dragging = false;
    if (node) node->addConnection(this);
    updatePath();
}

void ConnectionLine::setDragEnd(const QPointF &scenePos)
{
    m_dragging = true;
    m_dragEnd  = scenePos;
    updatePath();
}

void ConnectionLine::updatePath()
{
    QPointF start, end;

    if (m_srcNode && m_srcPin >= 0)
        start = m_srcNode->pinScenePos(m_srcPin);

    if (m_dragging) {
        end = m_dragEnd;
    } else if (m_dstNode && m_dstPin >= 0) {
        end = m_dstNode->pinScenePos(m_dstPin);
    } else {
        return;
    }

    rebuildCurve(start, end);
}

void ConnectionLine::rebuildCurve(const QPointF &start, const QPointF &end)
{
    QPainterPath p;
    p.moveTo(start);

    const qreal dx = qAbs(end.x() - start.x()) * 0.5;
    const qreal ctrlOffset = qMax(dx, 60.0);

    QPointF c1(start.x() + ctrlOffset, start.y());
    QPointF c2(end.x()   - ctrlOffset, end.y());
    p.cubicTo(c1, c2, end);

    setPath(p);
}

void ConnectionLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QPen p = pen();
    if (isSelected()) {
        p.setColor(QColor(100, 180, 255));
        p.setWidthF(4.0);
    }
    painter->setPen(p);
    painter->drawPath(path());
}

