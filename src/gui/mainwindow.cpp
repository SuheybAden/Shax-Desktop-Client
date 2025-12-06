#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../backend/node.h"

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

    // Load settings values
    marginOfError = settings.value("marginOfError", marginOfError_default).toFloat();
    mode = settings.value("mode", mode_default).toString();
    url = settings.value("url", url_default).toString();

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
    QObject::connect(ui->backBtn3, &QPushButton::clicked, this, &MainWindow::backBtnClicked);
    QObject::connect(ui->startGameBtn, &QPushButton::clicked, this, &MainWindow::startGameBtnClicked);
    QObject::connect(ui->joinLobbyBtn, &QPushButton::clicked, this, &MainWindow::joinLobbyBtnClicked);
    QObject::connect(ui->createLobbyBtn, &QPushButton::clicked, this, &MainWindow::createLobbyBtnClicked);
    QObject::connect(ui->gameBtn, &QPushButton::clicked, this, &MainWindow::gameBtnClicked);
    QObject::connect(ui->settingsBtn, &QPushButton::clicked, this, &MainWindow::settingsButtonClicked);
    QObject::connect(ui->saveSettingsBtn, &QPushButton::clicked, this, &MainWindow::saveSettingsButtonClicked);

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
    playerColors[0] = p1_color;
    playerColors[1] = p2_color;

    QPen linesPen(linesColor, penWidth);
    QPen nodesPen(linesColor, nodesBorderThickness);

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
        QPoint p = i.key();

        float x = p.x() * gridSpacing;
        float y = p.y() * gridSpacing;

        Node *node = new Node(x, y, radius, nodesPen, nodesBrush);

        connect(node, &Node::nodeClicked, this, &MainWindow::nodeClickedHandler);

        scene->addItem(node);
    }

    qDebug() << "Finished drawing the board.\n";
}


// ************************* TEXT-RELATED FUNCTIONS ************************ //

// Updates any text related to when there isn't a game running
// Ex: when in the settings menu, selecting a game mode, etc.
void MainWindow::updateIdleUI(){
    // Get the visible widget in the stacked widget
    QWidget *current = ui->stackedWidget->currentWidget();

    // Update the announcement for when the user isn't on the gameInfoFrame
    if (ui->gameInfoFrame_page != current) {
        if (!boardManager->status) {
            ui->announcementLbl->setText(tr("SERVOR ERROR!"));
            ui->statusbar->showMessage(tr("ERROR: Not connected to the server!"));
        }
        else {
            ui->announcementLbl->setText(tr("Start a New Game!"));
        }
    }
}

// Updates any text that tells the user the current state of the game
void MainWindow::updateGameInfoUI(QString nextState, int nextPlayer, QString msg, uint8_t flag, bool waiting){
    // Tell the user who the next player is
    if (settings.value("mode", "Local") == "Local") {
        ui->announcementLbl->setText(tr("Player %1's Turn.").arg(QString::number(nextPlayer + 1)));
    }
    else if (nextPlayer == boardManager->playerNum) {
        ui->announcementLbl->setText(tr("Your Turn!"));
    }
    else {
        ui->announcementLbl->setText(tr("Opponent's Turn."));
    }
    
    // Update the count of player tokens
    ui->p1PiecesLbl->setText(QString::number(boardManager->totalPieces[0]));
    ui->p2PiecesLbl->setText(QString::number(boardManager->totalPieces[1]));

    // Game state related UI updates
    if (nextState == "PLACEMENT"){
        ui->gameStateLbl->setText(tr("Place a Piece"));
        ui->gameBtn->setText(tr("Quit"));
    }
    else if (nextState == "REMOVAL" || nextState == "FIRST_REMOVAL"){
        ui->gameStateLbl->setText(tr("Remove a Piece"));
    }
    else if (nextState == "MOVEMENT"){
        ui->gameStateLbl->setText(tr("Move a Piece"));
    }
    else if (nextState == "STOPPED") {
        if (waiting) {
            ui->announcementLbl->setText(tr("Waiting for an opponent..."));
            ui->gameStateLbl->setText(tr("In Queue..."));
            ui->gameBtn->setText(tr("Quit"));

            if(boardManager->lobbyKey != 0) {
                ui->statusbar->showMessage(tr("Your lobby key is %1").arg(QString::number(boardManager->lobbyKey)));
            }
        }
        else {
            ui->gameStateLbl->setText(tr("Game Over"));
            ui->gameBtn->setText(tr("New Game"));

            // The player quit from the waiting list
            if (flag == 0x1){
                ui->announcementLbl->setText(tr("You've exited the waiting list!"));
            }

            // One of the player's won
            else if (flag == 0x2) {
                // If it's a local game, clarify which player won using their player number
                if (settings.value("mode", "Local") == "Local") {
                    ui->announcementLbl->setText(tr("Player %1 Won!").arg(QString::number(boardManager->winner + 1)));
                }
                // Otherwise, just clarify whether the user or their opponent forfeited
                else {
                    if (boardManager->winner == boardManager->playerNum) {
                        ui->announcementLbl->setText(tr("You Won!"));
                    }
                    else {
                        ui->announcementLbl->setText(tr("You Lost"));
                    }
                }
            }

            // One of the player's quit
            else if (flag == 0x3) {
                // If it's a local game, clarify which player won using their player number
                if (settings.value("mode", "Local") == "Local") {
                    ui->announcementLbl->setText(tr("Player %1 Forfeited").arg(QString::number(boardManager->currentTurn + 1)));
                }
                // Otherwise, just clarify whether the user or their opponent forfeited
                else {
                    if (boardManager->winner == boardManager->playerNum) {
                        ui->announcementLbl->setText(tr("Your Opponent Forfeited"));
                    }
                    else {
                        ui->announcementLbl->setText(tr("You Forfeited"));
                    }
                }
            }

            // One of the player's disconnected
            else if (flag == 0x4) {
                if (settings.value("mode", "Local") != "Local" && boardManager->winner == boardManager->playerNum) {
                    ui->announcementLbl->setText(tr("Your Opponent Disconnected"));
                }
                else {
                    ui->announcementLbl->setText(tr("Lost connection to the server"));
                }
            }

            // Unkown state
            else {
                ui->announcementLbl->setText(tr("Game ended in an unknown state"));
            }
        }
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
    ui->announcementLbl->setText(tr("Connecting to the server..."));
    boardManager->reconnect();
}

// Tries to create a private lobby
void MainWindow::createLobbyBtnClicked(){
    // Update the settings
    settings.setValue("mode", "Private Lobby");
    settings.setValue("lobby_key", 0);

    // Reconnect to the API server
    ui->announcementLbl->setText(tr("Connecting to the server..."));
    boardManager->reconnect();
}

// Tries to join a private lobby
void MainWindow::joinLobbyBtnClicked(){

    // Update the settings
    settings.setValue("mode", "Private Lobby");
    settings.setValue("lobby_key", ui->lobbyKeySpinBox->value());

    // Reconnect to the API server
    ui->announcementLbl->setText(tr("Connecting to the server..."));
    boardManager->reconnect();
}

void MainWindow::settingsButtonClicked(){
    // Update the settings values shown
    ui->urlLineEdit->setText(settings.value("url", "ws://localhost:8765").toString());

    // Transition to the settings page
    animatePageTransition(ui->settings_page, RIGHT);
}

// Save the user's settings
void MainWindow::saveSettingsButtonClicked(){
    settings.setValue("url", ui->urlLineEdit->text());

    QMessageBox::information(this, tr("Saved Settings"), tr("Your settings have been saved!"));
}

void MainWindow::animatePageTransition(QWidget *next, Direction transitionFrom){
    QWidget *current = ui->stackedWidget->currentWidget();

    if (!current || !next || current == next){
        return;
    }

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
    animCurrent->setDuration(pageTransitionTime);
    animCurrent->setStartValue(QRect(0, 0, w, ui->stackedWidget->height()));
    animCurrent->setEndValue(QRect(transitionFrom * -w, 0, w, ui->stackedWidget->height()));
    animCurrent->setEasingCurve(QEasingCurve::InOutQuad);

    QPropertyAnimation *fadeOutAnimation = new QPropertyAnimation(fadeOut, "opacity");
    fadeOutAnimation->setDuration(pageTransitionTime);
    fadeOutAnimation->setStartValue(1.0);
    fadeOutAnimation->setEndValue(0.0);
    fadeOutAnimation->setEasingCurve(QEasingCurve::OutQuad);

    // Animate next widget sliding in and fading in
    QPropertyAnimation *animNext = new QPropertyAnimation(next, "geometry");
    animNext->setDuration(pageTransitionTime);
    animNext->setStartValue(QRect(transitionFrom * w, 0, w, ui->stackedWidget->height()));
    animNext->setEndValue(QRect(0, 0, w, ui->stackedWidget->height()));
    animNext->setEasingCurve(QEasingCurve::InOutQuad);

    QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(fadeIn, "opacity");
    fadeInAnimation->setDuration(pageTransitionTime);
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

        updateIdleUI();
    });

    // Start all the animations
    animCurrent->start();
    animNext->start();
    fadeInAnimation->start();
    fadeOutAnimation->start();
}

void MainWindow::nodeClickedHandler(QObject *object) {
    if(boardManager->gameState != GameState::PLACEMENT) {
        return;
    }

    Node *node = (Node *)object;

    QPoint boardPos = sceneToBoard(node->nodePos);
    boardManager->placePiece(boardPos.x(), boardPos.y());
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
    ui->announcementLbl->setText(tr("Connected to Server."));

    // Message the API server
    boardManager->startGame();
}

void MainWindow::connectionErrorHandler(QString error) {
    ui->announcementLbl->setText(tr("Error connecting to server"));
    QMessageBox::critical(this, "Websocket Error", error);
}

void MainWindow::startGameResponseHandler(bool success, QString error, bool waiting, uint64_t lobbyKey, QString nextState, uint8_t nextPlayer, QHash<QPoint, QList<QPoint>> adjacentPieces){
    // Update the game-related text
    updateGameInfoUI(nextState, nextPlayer, "", 0, waiting);

    if (!success) {
        qDebug() << "Error" << error;
        QMessageBox::critical(this, tr("Error starting a new game"), error);
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
    // Update the game-related text
    updateGameInfoUI(nextState, nextPlayer, "", 0, false);

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
    // Update the game-related text
    updateGameInfoUI(nextState, nextPlayer, "", 0, false);

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
    // Update the game-related text
    updateGameInfoUI(nextState, nextPlayer, "", 0, false);

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
    
    // Update the game-related text
    updateGameInfoUI("STOPPED", boardManager->currentTurn, "", flag, waiting);

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
