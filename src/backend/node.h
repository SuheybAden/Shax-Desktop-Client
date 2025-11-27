#ifndef NODE_H
#define NODE_H

#include <QGraphicsObject>
#include <QObject>
#include <QVariant>
#include <QPen>
#include <QBrush>


class Node : public QGraphicsObject
{
    Q_OBJECT
public:
    Node(float x, float y, float radius, QPen pen, QBrush brush);

    float radius;
    QPen pen;
    QBrush brush;
    QPointF nodePos;

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

signals:
    void nodeClicked(QObject* node);

private:
    // Event Handlers
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
 };

#endif // NODE_H
