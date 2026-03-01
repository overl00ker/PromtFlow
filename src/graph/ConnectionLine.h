#pragma once

#include <QGraphicsPathItem>

class NodeItem;

class ConnectionLine : public QGraphicsPathItem
{
public:
    ConnectionLine(QGraphicsItem *parent = nullptr);

    void setSource(NodeItem *node, int pinIndex);
    void setTarget(NodeItem *node, int pinIndex);

    NodeItem *sourceNode() const { return m_srcNode; }
    NodeItem *targetNode() const { return m_dstNode; }
    int       sourcePin()  const { return m_srcPin;  }
    int       targetPin()  const { return m_dstPin;  }

    void updatePath();
    void setDragEnd(const QPointF &scenePos);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    void rebuildCurve(const QPointF &start, const QPointF &end);

    NodeItem *m_srcNode = nullptr;
    NodeItem *m_dstNode = nullptr;
    int       m_srcPin  = -1;
    int       m_dstPin  = -1;

    QPointF   m_dragEnd;
    bool      m_dragging = false;
};
