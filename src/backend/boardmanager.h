#ifndef BOARDMANAGER_H
#define BOARDMANAGER_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <stdint.h>
#include <QPointF>
#include <QPoint>
#include <QHash>
#include <QList>
#include <QSettings>

enum GameState{
    STOPPED,
    PLACEMENT,
    REMOVAL,
    FIRST_REMOVAL,
    MOVEMENT
};

class BoardManager : public QObject
{
    Q_OBJECT
public:
    explicit BoardManager(QSettings *settings, QObject *parent = nullptr);
    ~BoardManager();

    QSettings *settings;
    bool running = false;
    bool waiting = false;
    bool status = false;
    uint8_t currentTurn = 0;
    uint16_t totalPieces[2] = {0, 0};
    uint8_t firstToJare = 0;
    uint8_t currentJare[2] = {0, 0};

    uint8_t gameType = 0;
    uint8_t playerNum = 0;
    uint8_t winner = 0;
    uint8_t playerTokens[2] = {0, 0};

    GameState gameState = GameState::STOPPED;
    QWebSocket websocket;
    QUrl url;
    QString mode;

public slots:
    void onConnected();
    void onTextMessageReceived(const QString &msg);
    void error(QAbstractSocket::SocketError error);

    void startGame();
    void placePiece(uint8_t x, uint8_t y);
    void removePiece(uint16_t pieceId);
    void movePiece(uint16_t pieceId, uint8_t x, uint8_t y);
    void quitGame();

    void reconnect();

signals:
    void connected();
    void connectionError(QString error);
    void startGameResponded(bool success, QString error, bool waiting, QString nextState, uint8_t nextPlayer, QHash<QPoint, QList<QPoint>> adjacentPieces);
    void placePieceResponded(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void removePieceResponded(bool success, QString error, uint16_t ID, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void movePieceResponded(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void quitGameResponded(bool success, QString msg, uint8_t winner, uint8_t flag, bool waiting);

private:
    const uint8_t TOTAL_PLAYERS = 2;
    const uint8_t MAX_PIECES = 12;
    const uint8_t MIN_PIECES = 2;
    const double MARGIN_OF_ERROR = 0.2;
    const uint8_t ID_SHIFT = 1;

    void connectSignals();
    void sendMessage(QJsonObject msg);

    QJsonObject loadJson(QString msg);
    QString dumpJson(QJsonObject msg);
    void setState(QString s);

    void startGameResponseHandler(QJsonObject &data);
    void placePieceResponseHandler(QJsonObject &data);
    void removePieceResponseHandler(QJsonObject &data);
    void movePieceResponseHandler(QJsonObject &data);
    void quitGameResponseHandler(QJsonObject &data);
};

#endif // BOARDMANAGER_H
