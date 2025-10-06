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
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
//    ui->printLbl->setVisible(false);

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
    QObject::connect(ui->findGameBtn, &QPushButton::clicked, this, &MainWindow::findGameBtnClicked);
    QObject::connect(ui->lobbyBtn, &QPushButton::clicked, this, &MainWindow::lobbyBtnClicked);
    QObject::connect(ui->backBtn1, &QPushButton::clicked, this, &MainWindow::backBtnClicked);
    QObject::connect(ui->backBtn2, &QPushButton::clicked, this, &MainWindow::backBtnClicked);
    QObject::connect(ui->startGameBtn, &QPushButton::clicked, this, &MainWindow::startGameBtnClicked);
    QObject::connect(ui->joinLobbyBtn, &QPushButton::clicked, this, &MainWindow::joinLobbyBtnClicked);
    QObject::connect(ui->createLobbyBtn, &QPushButton::clicked, this, &MainWindow::createLobbyBtnClicked);
    QObject::connect(ui->gameBtn, &QPushButton::clicked, this, &MainWindow::gameBtnClicked);
    QObject::connect(ui->settingsButton, &QToolButton::clicked, this, &MainWindow::settingsButtonClicked);

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
    playerColors[0] = QColor(140, 75, 50);
    playerColors[1] = QColor(50, 50, 50);

    QPen linesPen(QColor(127,92,38), penWidth);
    QPen nodesPen(QColor(127,92,38), 2);
    QBrush nodesBrush(QColor(200, 180, 150));

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

        scene->addEllipse(x - radius, y - radius, radius * 2, radius * 2, nodesPen, nodesBrush);
    }

    // Add an event filter to detect mouse button presses
    ui->graphicsView->installEventFilter(this);

    qDebug() << "Finished drawing the board.\n";
}


// ************************* TEXT-RELATED FUNCTIONS ************************ //
void MainWindow::updateOnScreenText(QString nextState, int nextPlayer, QString msg, uint8_t flag, bool waiting){

    // Update the announcement label
    if (!boardManager->status) {
        ui->announcementLbl->setText("Not connected to the server");
        return;
    }
    else if (boardManager->running) {
        if (settings.value("mode", "Local") == "Local") {
            ui->announcementLbl->setText("Player " + QString::number(nextPlayer + 1) + "'s Turn.");
        }
        else if (nextPlayer == boardManager->playerNum) {
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
            ui->announcementLbl->setText("Waiting for an opponent...");
            ui->gameBtn->setText("Quit");

            if(boardManager->lobbyKey != 0) {
                ui->printLbl->setText("Your lobby key is " + QString::number(boardManager->lobbyKey));
            }
        }
        else {
            ui->gameBtn->setText("New Game");
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

// *************************** UI EVENT HANDLERS ************************ //
void MainWindow::gameBtnClicked(){
    // Tries to end the running game
    if (boardManager->running || boardManager->waiting) {
        boardManager->quitGame();
    }

    // Otherwise, closes the game state frame and shows the game settings again
    else {
        animatePageTransition(ui->actionsFrame_page, LEFT);
    }
}

// Switches the visible UI frame to the startGameFrame
void MainWindow::findGameBtnClicked(){
    animatePageTransition(ui->startGameFrame_page, RIGHT);
}

// Switches the visible UI frame to the lobbyFrame
void MainWindow::lobbyBtnClicked(){
    animatePageTransition(ui->lobbyFrame_page, RIGHT);
}

// Returns to the initial actionsFrame
void MainWindow::backBtnClicked(){
    animatePageTransition(ui->actionsFrame_page, LEFT);
}

// Tries to find a game for the player
void MainWindow::startGameBtnClicked(){
    // Update the settings
    settings.setValue("mode", ui->gameTypeComboBox->currentText());
    settings.setValue("lobby_key", 0);

    // Reconnect to the API server
    ui->announcementLbl->setText("Connecting to the server...");
    boardManager->reconnect();
}

// Tries to create a private lobby
void MainWindow::createLobbyBtnClicked(){
    // Update the settings
    settings.setValue("mode", "Private Lobby");
    settings.setValue("lobby_key", 0);

    // Reconnect to the API server
    ui->announcementLbl->setText("Connecting to the server...");
    boardManager->reconnect();
}

// Tries to join a private lobby
void MainWindow::joinLobbyBtnClicked(){

    // Update the settings
    settings.setValue("mode", "Private Lobby");
    settings.setValue("lobby_key", ui->lobbyKeySpinBox->value());

    // Reconnect to the API server
    ui->announcementLbl->setText("Connecting to the server...");
    boardManager->reconnect();
}

void MainWindow::settingsButtonClicked(){
    if (boardManager->running || boardManager->waiting)
        QMessageBox::critical(this, "Ongoing Game", "The current game must be finished before editing the settings.");

    else {
        qDebug() << "Opened the settings window.\n";
        SettingsWindow settingsWindow(&(this->settings));
        if (settingsWindow.exec()){
            qDebug() << "Your settings were saved!\n";
        }
    }
}

void MainWindow::animatePageTransition(QWidget *next, Direction transitionFrom){
    QWidget *current = ui->stackedWidget->currentWidget();

    if (!current || !next || current == next){
        return;
    }

    int transitionTime = 400;
    int w = ui->stackedWidget->width();
    QGraphicsOpacityEffect *fadeIn = new QGraphicsOpacityEffect();
    QGraphicsOpacityEffect *fadeOut = new QGraphicsOpacityEffect();
    current->setGraphicsEffect(fadeOut);
    next->setGraphicsEffect(fadeIn);

    // Start position: slide in from right
    next->setGeometry(transitionFrom * w, 0, w, ui->stackedWidget->height());
    next->show();

    // Animate current widget sliding out and fading out
    QPropertyAnimation *animCurrent = new QPropertyAnimation(current, "geometry");
    animCurrent->setDuration(transitionTime);
    animCurrent->setStartValue(QRect(0, 0, w, ui->stackedWidget->height()));
    animCurrent->setEndValue(QRect(transitionFrom * -w, 0, w, ui->stackedWidget->height()));
    animCurrent->setEasingCurve(QEasingCurve::InOutQuad);

    QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(fadeOut, "opacity");
    fadeOutAnimation->setDuration(transitionTime);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);
    fadeOutAnimation->setEasingCurve(QEasingCurve::OutQuad);

    // Animate next widget sliding in and fading in
    QPropertyAnimation *animNext = new QPropertyAnimation(next, "geometry");
    animNext->setDuration(transitionTime);
    animNext->setStartValue(QRect(transitionFrom * w, 0, w, ui->stackedWidget->height()));
    animNext->setEndValue(QRect(0, 0, w, ui->stackedWidget->height()));
    animNext->setEasingCurve(QEasingCurve::InOutQuad);

    QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(fadeIn, "opacity");
    fadeInAnimation->setDuration(transitionTime);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);
    fadeInAnimation->setEasingCurve(QEasingCurve::OutQuad);


    // Delete any data related to the animation once it has completed
    // Update the current page in the stackedWidget and hide the previous page
    QObject::connect(animNext, &QPropertyAnimation::finished, [=]() {
        ui->stackedWidget->setCurrentWidget(next);
        current->hide();
        animCurrent->deleteLater();
        animNext->deleteLater();
        fadeIn->deleteLater();
        fadeOut->deleteLater();
        fadeInAnimation->deleteLater();
        fadeOutAnimation->deleteLater();
    });

    // Start all the animations
    animCurrent->start();
    animNext->start();
    fadeInAnimation->start();
    fadeOutAnimation->start();
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

    // Message the API server
    boardManager->startGame();
}

void MainWindow::connectionErrorHandler(QString error) {
    ui->announcementLbl->setText("Error connecting to server");
    QMessageBox::critical(this, "Websocket Error", error);
}

void MainWindow::startGameResponseHandler(bool success, QString error, bool waiting, uint64_t lobbyKey, QString nextState, uint8_t nextPlayer, QHash<QPoint, QList<QPoint>> adjacentPieces){
    // Update on screen text
    updateOnScreenText(nextState, nextPlayer, "", 0, waiting);

    if (!success) {
        qDebug() << "Error" << error;
        QMessageBox::critical(this, "Error starting a new game", error);
        return;
    }

    // Shows the game info frame and hides the other frames
    animatePageTransition(ui->gameInfoFrame_page, RIGHT);

    if (waiting) {
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
