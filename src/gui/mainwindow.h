#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QPointF>
#include <QPoint>
#include <QHash>
#include <QList>
#include <QColor>
#include "../backend/boardmanager.h"
#include "../backend/gamepiece.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // API response handlers
    void connectedToBoard();
    void connectionErrorHandler(QString error);
    void startGameResponseHandler(bool success, QString error, bool waiting, uint64_t lobbyKey, QString nextState, uint8_t nextPlayer,  QHash<QPoint, QList<QPoint>> adjacentPieces);
    void placePieceResponseHandler(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void removePieceResponseHandler(bool success, QString error, uint16_t ID, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void movePieceResponseHandler(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces);
    void quitGameResponseHandler(bool success, QString error, uint8_t winner, uint8_t flag, bool waiting);

private:
    Ui::MainWindow *ui;

    const QString loadingGifPath = ":/images/loading.gif";
    QGraphicsScene *scene;
    QGraphicsProxyWidget *loadingWidget;
    QSettings settings = QSettings("SA LLC", "Shax Desktop Client");

    QWidget currentFrame;

    float marginOfError;
    QString mode;
    QString url;

    BoardManager *boardManager;

    // Graphics parameters
    float radius = 15;
    float penWidth = 3;
    float gridSpacing = 70;
    QColor playerColors[2];
    QColor colorBlack;
    QColor colorRed;
    QColor colorBlue;

    QHash<QPoint, QList<QPoint>> adjacentPieces;
    QHash<uint16_t,GamePiece*> gamePieces;

    // Init methods
    void connectAll();
    void initBoard(QHash<QPoint, QList<QPoint>> adjacentPieces);

    // Event handlers
//    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    void gamePieceReleased(QObject *object);
    void findGameBtnClicked();
    void lobbyBtnClicked();
    void backBtnClicked();
    void startGameBtnClicked();
    void joinLobbyBtnClicked();
    void createLobbyBtnClicked();
    void gameBtnClicked();
    void settingsButtonClicked();


    // Update methods
    void updateOnScreenText(QString nextState, int nextPlayer, QString msg, uint8_t flag, bool waiting);

    // Board translation methods
    QPoint sceneToBoard(QPointF scenePoint);
    QPointF boardToScene(QPoint boardPoint);

    // Highlights movable/removable pieces
    void highlightPieces(QList<uint16_t> activePieces, bool isMovable);
};
#endif // MAINWINDOW_H
