#ifndef GAMEPIECE_H
#define GAMEPIECE_H

#include <QGraphicsObject>
#include <QObject>
#include <QVariant>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemAnimation>
#include <QTimeLine>


class GamePiece : public QGraphicsObject
{
    Q_OBJECT
public:
    GamePiece(uint16_t ID, float x, float y, float radius, QColor color);

    uint16_t ID;
    float radius;
    QColor color;
    QPointF homePos;
    QPointF currentPos;

    void activate(bool isMovable);
    void deactivate();

    void movePiece(int16_t x, int16_t y);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

signals:
    void pieceReleased(QObject* piece);

private:
    bool movable;
    bool removable;

    // Event Handlers
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    // Animation variables
    QTimeLine *timer;
    QGraphicsItemAnimation *animation;
};

#endif // GAMEPIECE_H
