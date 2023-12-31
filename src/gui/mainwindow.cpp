#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "settingswindow.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QMovie>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Load settings values
    marginOfError = settings.value("marginOfError", 0.2).toFloat();
    mode = settings.value("mode", "Remote").toString();
    url = settings.value("url", "ws://localhost:8765").toString();

    // Initialize colors
    colorBlack = QColor(0, 0, 0);
    colorRed = QColor(150, 0, 0);
    colorBlue = QColor(0, 0, 150);

    // Add a blank scene to the graphics view
    scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);

    // Init boardManager
    boardManager = new BoardManager(&settings);

    connectAll();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete boardManager;
    delete scene;
}


// ************************** INIT FUNCTIONS ************************* //
void MainWindow::connectAll(){
    // Connect signals from UI elements
    QObject::connect(ui->gameBtn, &QPushButton::clicked, this, &MainWindow::gameBtnClicked);
    QObject::connect(ui->settingsAction, &QAction::triggered, this, &MainWindow::settingsActionTriggered);

    // Connect signals from the board manager
    QObject::connect(boardManager, &BoardManager::connected, this, &MainWindow::connectedToBoard);
    QObject::connect(boardManager, &BoardManager::connectionError, this, &MainWindow::connectionErrorHandler);
    QObject::connect(boardManager, &BoardManager::startGameResponded, this, &MainWindow::startGameResponseHandler);
    QObject::connect(boardManager, &BoardManager::placePieceResponded, this, &MainWindow::placePieceResponseHandler);
    QObject::connect(boardManager, &BoardManager::removePieceResponded, this, &MainWindow::removePieceResponseHandler);
    QObject::connect(boardManager, &BoardManager::movePieceResponded, this, &MainWindow::movePieceResponseHandler);
    QObject::connect(boardManager, &BoardManager::quitGameResponded, this, &MainWindow::quitGameResponseHandler);
}

void MainWindow::initBoard(QHash<QPoint, QList<QPoint>> adjacentPieces){
    playerColors[0] = QColor(220, 220, 220); //QColor(181, 189, 165);
    playerColors[1] = QColor(50, 50, 50); //QColor(57, 61, 71);

    QPen linesPen(colorBlack, penWidth);
    QPen nodesPen(colorBlack, 2);
    QBrush brush(QColor(170, 120, 80));

    // Clears all items from the scene
    scene->clear();

    // Draw the lines in between the nodes
    for (auto i = adjacentPieces.cbegin(), end = adjacentPieces.cend(); i != end; ++i) {
        QPoint node = i.key();
        QList<QPoint> neighbors = i.value();

        float x1 = node.x();
        float y1 = node.y();

        for (const auto& neighbor: neighbors) {
            float x2 = neighbor.x();
            float y2 = neighbor.y();

            scene->addLine(x1 * gridSpacing, y1 * gridSpacing, x2 * gridSpacing, y2 * gridSpacing, linesPen);
        }
    }

    // Draw the nodes themselves
    for (auto i = adjacentPieces.cbegin(), end = adjacentPieces.cend(); i != end; ++i) {
        QPoint node = i.key();

        float x = node.x() * gridSpacing;
        float y = node.y() * gridSpacing;

        scene->addEllipse(x - radius, y - radius, radius * 2, radius * 2, nodesPen, brush);
    }

    // Add an event filter to detect mouse button presses
    ui->graphicsView->installEventFilter(this);

    qDebug() << "Finished drawing the board.\n";
}


// ************************* TEXT-RELATED FUNCTIONS ************************ //
void MainWindow::updateOnScreenText(QString nextState, int nextPlayer, QString msg, uint8_t flag, bool waiting){

    if (!boardManager->status) {
        ui->announcementLbl->setText("Not connected to the server");
        return;
    }

    // Update the announcement label
    if (settings.value("mode", "Local") == "Local")
        ui->announcementLbl->setText("Player " + QString::number(nextPlayer + 1) + "'s Turn.");
    else {
        if (nextPlayer == boardManager->playerNum) {
            ui->announcementLbl->setText("Your Turn!");
        }
        else {
            ui->announcementLbl->setText("Opponent's Turn.");
        }
    }

    // Update the count of player tokens
    ui->p1PiecesLbl->setText(QString::number(boardManager->totalPieces[0]));
    ui->p2PiecesLbl->setText(QString::number(boardManager->totalPieces[1]));

    // Game state related UI updates
    if (nextState == "STOPPED") {
        ui->gameStateLbl->setText("Game Stopped");
        ui->printLbl->setText("");

        if (waiting) {
            ui->announcementLbl->setText("Waiting for a game...");
            ui->gameBtn->setText("Quit");
        }
        else {
            ui->gameBtn->setText("Start Game");
            switch (flag) {

                case 0x0:
                    break;

                // The player quit from the waiting list
                case 0x1:
                    ui->announcementLbl->setText("Exited the waiting list");
                    break;

                // One of the player's won
                case 0x2:
                    if (settings.value("mode", "Local") == "Local")
                        ui->announcementLbl->setText("Player " + QString::number(boardManager->winner + 1) + " Won!");
                    else {
                        if (boardManager->winner == boardManager->playerNum) {
                            ui->announcementLbl->setText("Your Won!");
                        }
                        else {
                            ui->announcementLbl->setText("You Lost");
                        }
                    }
                    break;

                // One of the player's quit
                case 0x3:
                    if (settings.value("mode", "Local") == "Local")
                        ui->announcementLbl->setText("Player " + QString::number(boardManager->winner + 1) + " Won!");
                    else {
                        if (boardManager->winner == boardManager->playerNum) {
                            ui->announcementLbl->setText("Your Opponent Forfeited");
                        }
                        else {
                            ui->announcementLbl->setText("You Forfeited");
                        }
                    }
                    break;

                // One of the player's disconnected
                case 0x4:
                    if (settings.value("mode", "Local") == "Local")
                        ui->announcementLbl->setText("Lost the connection to the shax api");
                    else {
                        if (boardManager->winner == boardManager->playerNum) {
                            ui->announcementLbl->setText("Your Opponent Disconnected");
                        }
                        else {
                            ui->announcementLbl->setText("You've disconnected from the shax api");
                        }
                    }
                    break;

                // Unkown state
                default:
                    ui->announcementLbl->setText("Game ended in an unknown state");
                    break;
            }
        }
    }
    else if (nextState == "PLACEMENT"){
        ui->gameStateLbl->setText("Placement Stage");
        ui->printLbl->setText("Click on a node to place a piece.");
        ui->gameBtn->setText("Quit");
    }
    else if (nextState == "REMOVAL" || nextState == "FIRST_REMOVAL"){
        ui->gameStateLbl->setText("Removal Stage");
        ui->printLbl->setText("Click on a piece to remove");
    }
    else if (nextState == "MOVEMENT"){
        ui->gameStateLbl->setText("Movement Stage");
        ui->printLbl->setText("Drag one of your pieces to an adjacent spot");
    }
}

// *************************** EVENT HANDLERS ************************ //
void MainWindow::gameBtnClicked(){
    // Tries to end the game
    if (boardManager->running || boardManager->waiting) {
        boardManager->quitGame();
    }

    // Tries to start a game
    else {
        boardManager->gameType = 0;
        boardManager->startGame();
    }
}

void MainWindow::settingsActionTriggered(){
    if (boardManager->running || boardManager->waiting)
        QMessageBox::critical(this, "Ongoing Game", "The current game must be finished before editing the settings.");

    else {
        qDebug() << "Opened the settings window.\n";
        SettingsWindow settingsWindow(&(this->settings));
        if (settingsWindow.exec()){
            qDebug() << "Your settings were saved!\n";
            ui->announcementLbl->setText("Reconnecting to the server...");
            boardManager->reconnect();
        }
    }
}


bool MainWindow::eventFilter(QObject *object, QEvent *event){
//    qDebug() << "Event type: " << event->type() << "Object type: " << object->objectName();

    if (event->type() == QEvent::MouseButtonPress && boardManager->gameState == GameState::PLACEMENT) {
        QMouseEvent *mouseEvent = (QMouseEvent *)event;
        QPointF pos = ui->graphicsView->mapToScene(mouseEvent->pos());
        QPoint boardPos = sceneToBoard(pos);

        boardManager->placePiece(boardPos.x(), boardPos.y());
        event->accept();
        return true;
    }

    return false;
//    return QMainWindow::eventFilter(object, event);
}


void MainWindow::gamePieceReleased(QObject *object){
    GamePiece *piece = (GamePiece *)object;

    if(boardManager->gameState == GameState::FIRST_REMOVAL || boardManager->gameState == GameState::REMOVAL){
        boardManager->removePiece(piece->ID);
    }

    else if(boardManager->gameState == GameState::MOVEMENT) {
        QPoint boardPos = sceneToBoard(piece->currentPos);
        qDebug() << "Piece moved to" << boardPos;
        if(boardPos != QPoint(-1, -1)){
            boardManager->movePiece(piece->ID, boardPos.x(), boardPos.y());
        }
        else {
            piece->movePiece(-1, -1);
        }
    }
}

// *************************** API RESPONSE HANDLERS ********************** //
void MainWindow::connectedToBoard(){
    ui->announcementLbl->setText("Connected to Server.");
}

void MainWindow::connectionErrorHandler(QString error) {
    ui->announcementLbl->setText("Error connecting to server");
    QMessageBox::critical(this, "Websocket Error", error);
}

void MainWindow::startGameResponseHandler(bool success, QString error, bool waiting, QString nextState, uint8_t nextPlayer, QHash<QPoint, QList<QPoint>> adjacentPieces){
    // Update on screen text
    updateOnScreenText(nextState, nextPlayer, "", 0, waiting);

    if (!success) {
        qDebug() << "Error" << error;
        return;
    }

    if (waiting) {
//        QFile *file = new QFile(loadingGifPath);
//        qDebug() << file->exists() << QDir::current().absolutePath();

        QMovie *movie = new QMovie(loadingGifPath);
        movie->setScaledSize(QSize(100, 100));

        QLabel *loadingGif = new QLabel();
        loadingGif->setAttribute(Qt::WA_TranslucentBackground);
        loadingGif->setMovie(movie);

        movie->start();
        scene = ui->graphicsView->scene();

        // Clear the scene and add the loading widget
        scene->clear();
        scene->addWidget(loadingGif);
    }

    else {
        initBoard(adjacentPieces);
    }

}

void MainWindow::placePieceResponseHandler(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces){
    // Update on screen text
    updateOnScreenText(nextState, nextPlayer, "", 0, false);

    // Check if the piece placement has been approved
    if (!success) {
        return;
    }

    // Get the properties of the new game piece
    QPointF p = boardToScene(QPoint(x, y));
    uint8_t player = ID & 0x1;

    qDebug() << "Placing piece at:" << p;

    // Create a new game piece
    GamePiece *newPiece = new GamePiece(ID, p.x(), p.y(), radius, playerColors[player]);

    // Add the piece to the scene and move it to its home position
    scene->addItem(newPiece);

    // Store the piece for future use
    gamePieces.insert(ID, newPiece);

    // Connect the signals from the game piece
    connect(newPiece, &GamePiece::pieceReleased, this, &MainWindow::gamePieceReleased);

    // Highlight any removable pieces
    if(nextState == "FIRST_REMOVAL")
        highlightPieces(activePieces, false);
}

void MainWindow::removePieceResponseHandler(bool success, QString error, uint16_t ID, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces){
    // Update on screen text
    updateOnScreenText(nextState, nextPlayer, "", 0, false);

    if (!success){
        qDebug() << "The piece couldn't be moved: " << error;
        return;
    }

    // Remove the piece from the scene
    GamePiece* piece = gamePieces.take(ID);
    scene->removeItem(piece);
    delete piece;

    // Activates any pieces that could be removed/moved in the next stage
    qDebug() << activePieces;
    highlightPieces(activePieces, nextState == "MOVEMENT");
}

void MainWindow::movePieceResponseHandler(bool success, QString error, uint16_t ID, uint8_t x, uint8_t y, QString nextState, uint8_t nextPlayer, QList<uint16_t> activePieces){
    // Update on screen text
    updateOnScreenText(nextState, nextPlayer, "", 0, false);

    // Get the piece corresponding to the evaluated piece movement
    GamePiece* piece = gamePieces[ID];

    // If piece movement was not approved, move the piece back to its original position
    if (!success){
        qDebug() << "The piece couldn't be moved" << error;
        piece->movePiece(-1, -1);
        return;
    }

    // Otherwise move it to its new position
    QPointF p = boardToScene(QPoint(x, y));
    piece->movePiece(p.x(), p.y());

    qDebug() << "Moving piece to:" << p << ", Piece ID:" << ID;

    // Activates any pieces that can be moved in the next stage
    highlightPieces(activePieces, nextState == "MOVEMENT");
}

void MainWindow::quitGameResponseHandler(bool success, QString error, uint8_t winner, uint8_t flag, bool waiting){
    if (!success) {
        qDebug() << "Couldn't end the game: " << error;
        return;
    }

    updateOnScreenText("STOPPED", boardManager->currentTurn, "", flag, waiting);

    // Clear the scene if exiting the waiting list
    if(flag == 0x1) scene->clear();
}

// ************************* BOARD-SCENE TRANSLATIONS ********************** //
QPoint MainWindow::sceneToBoard(QPointF scenePoint){
    float raw_x = scenePoint.x() / gridSpacing;
    float raw_y = scenePoint.y() / gridSpacing;

    float error_x = abs(round(raw_x) - raw_x);
    float error_y = abs(round(raw_y) - raw_y);

    // TODO: double check if this is supposed to be an and or an or
    if (error_x > marginOfError && error_y > marginOfError)
        return QPoint(-1, -1);

    return QPoint(round(raw_x), round(raw_y));
}


QPointF MainWindow::boardToScene(QPoint boardPoint){
    return QPointF(boardPoint.x() * gridSpacing, boardPoint.y() * gridSpacing);
}


void MainWindow::highlightPieces(QList<uint16_t> activePieces, bool isMovable) {
    for (auto i = gamePieces.cbegin(), end = gamePieces.cend(); i != end; i++) {
        // Activates the game piece if it's in the activePieces list
        if (activePieces.contains(i.key())) {
            i.value()->activate(isMovable);
        }
        // Deactivate all other game pieces
        else
            i.value()->deactivate();
    }

    // Update the scene to show changes
    scene->update();
}
