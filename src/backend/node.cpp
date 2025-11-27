#include "node.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>


Node::Node(float x, float y, float radius, QPen pen, QBrush brush)
{
    this->radius = radius;
    this->pen = pen;
    this->brush = brush;

    this->nodePos = QPointF(x, y);

    setPos(nodePos);
}


// ********************************** OVERLOADS *********************************** //
QRectF Node::boundingRect() const{
    return QRectF(-radius, -radius, radius*2, radius*2);
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    QRectF rect = boundingRect();

    painter->setPen(pen);
    painter->setBrush(brush);

    painter->drawEllipse(rect);
}


// ********************************** EVENT HANDLERS ******************************** //
void Node::mousePressEvent(QGraphicsSceneMouseEvent *event){
    event->accept();
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit nodeClicked(this);
    QGraphicsObject::mouseReleaseEvent(event);
}
