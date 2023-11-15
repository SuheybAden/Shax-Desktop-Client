#include "gamepiece.h"
#include <QGraphicsScene>


GamePiece::GamePiece(uint16_t ID, float x, float y, float radius, QColor color)
{
    this->ID = ID;
    this->radius = radius;
    this->color = color;

    this->movable = false;
    this->removable = false;

    this->currentPos.rx() = x;
    this->currentPos.ry() = y;
    this->homePos.rx() = x;
    this->homePos.ry() = y;

    setPos(homePos);

    GamePiece::setAcceptTouchEvents(true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

void GamePiece::activate(bool isMovable){
    if (isMovable){
        setFlag(QGraphicsItem::ItemIsMovable);
    }
    movable = isMovable;
    removable = !isMovable;
}

void GamePiece::deactivate(){
    setFlag(QGraphicsItem::ItemIsMovable, false);

    movable = false;
    removable = false;
}


// ********************************** OVERLOADS *********************************** //
QRectF GamePiece::boundingRect() const{
    return QRectF(-radius, -radius/2, radius*2, radius);
}

void GamePiece::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    QRectF rect = boundingRect();

//    qDebug() << "Updated piece:" << ID;
//    qDebug() << "ID: " << ID << ", Movable: " << movable << ", Removable: " << removable;
    if (movable)
        painter->setPen(QPen(QColor(0,150,0), 2));
    else if (removable)
        painter->setPen(QPen(QColor(150, 0, 0), 2));
    else
        painter->setPen(QPen(QColor(0,0,0), 2));

    painter->setBrush(QBrush(color));

    painter->drawEllipse(rect);
}


// ********************************** EVENT HANDLERS ******************************** //
QVariant GamePiece::itemChange(GraphicsItemChange change, const QVariant &value){
    if (change == QGraphicsItem::ItemPositionChange && scene()){
        currentPos = value.toPointF();
        qDebug() << "Home: " << homePos << "\t Current: " << currentPos << "\t Value:" << value.toPointF();
    }

    return QGraphicsObject::itemChange(change, value);
}

void GamePiece::mousePressEvent(QGraphicsSceneMouseEvent *event){
    event->accept();
}

void GamePiece::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit pieceReleased(this);
}

void GamePiece::movePiece(int16_t x, int16_t y){
    if (x != -1 && y != -1) {
        this->homePos.setX(x);
        this->homePos.setY(y);
    }

    setPos(homePos);
}
