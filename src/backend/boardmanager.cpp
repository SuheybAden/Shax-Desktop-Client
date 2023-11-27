#include "boardmanager.h"
#include <stdio.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

BoardManager::BoardManager(QSettings *settings, QObject *parent)
    : QObject{parent}
{
    gameState = GameState::STOPPED;
    this->settings = settings;

    // Opens websocket connection to shax server
    mode = settings->value("mode", "Remote").toString();
    this->url = QUrl(settings->value("url", "ws://localhost:8765").toString());

    websocket.open(url);
    qDebug() << "Opened websocket";

    // Connects all signals and slots
    connectSignals();
}

// Closes websocket connection before deleting
BoardManager::~BoardManager(){
    websocket.close(QWebSocketProtocol::CloseCodeAbnormalDisconnection);
    qDebug() << "Closing connection";
}

void BoardManager::connectSignals(){
    QObject::connect(&websocket, &QWebSocket::connected, this, &BoardManager::onConnected);
    QObject::connect(&websocket, &QWebSocket::textMessageReceived, this, &BoardManager::onTextMessageReceived);
    QObject::connect(&websocket, &QWebSocket::errorOccurred, this, &BoardManager::error);
}

void BoardManager::sendMessage(QJsonObject msg){
    if(status){
        QString msgStr = dumpJson(msg);
        websocket.sendTextMessage(msgStr);
    }
    else
        qDebug() << "Not connected to the server yet\n";
}

void BoardManager::reconnect(){
    mode = settings->value("mode", "Remote").toString();
    this->url = QUrl(settings->value("url", "ws://localhost:8765").toString());

    websocket.open(url);
    qDebug() << "Attempting to reconnect...";
}

// ******************** JSON HELPER FUNCTIONS ***************************** //

QJsonObject BoardManager::loadJson(QString msg){
    QJsonDocument response = QJsonDocument::fromJson(msg.toUtf8());
    return response.object();
}
QString BoardManager::dumpJson(QJsonObject msg){
    return QJsonDocument(msg).toJson(QJsonDocument::Compact);
}


// ********************* WEBSOCKET-RELATED SLOTS **************************** //
void BoardManager::onConnected(){
    status = true;
    emit connected();
}

void BoardManager::onTextMessageReceived(const QString &msg){
    qDebug() << "Got a response.";

    QJsonObject data = loadJson(msg);

    // Get the current action of the
    QString action = "";
    if(const QJsonValue v = data["action"]; v.isString()){
        action = v.toString();
    }

    if (action == "join_game")
        startGameResponseHandler(data);
    else if(action == "place_piece")
        placePieceResponseHandler(data);
    else if(action == "remove_piece")
        removePieceResponseHandler(data);
    else if(action == "move_piece")
        movePieceResponseHandler(data);
    else if(action == "quit_game")
        quitGameResponseHandler(data);
    else
        qDebug() << "Received unexpected action:" << action << "\n";
}

void BoardManager::error(QAbstractSocket::SocketError error){
    qDebug() << "An error has occured:" << error << "\n";

    // Create an artificial API response to properly end the current game
    if (status) {
        QJsonObject data;
        data["success"] = true;
        data["error"] = "";
        data["winner"] = 0;
        data["forfeit"] = true;
        quitGameResponseHandler(data);
    }

    status = false;
    running = false;
    waiting = false;

    // Handles a lost connection
    if (error == QAbstractSocket::RemoteHostClosedError) {
        emit connectionError("Lost the connection to the websocket server. Check your connection and try again.");

    }
    else if (error == QAbstractSocket::ConnectionRefusedError){
        emit connectionError("Couldn't connect to the websocket server.");
    }
    else {
        emit connectionError("An error occured with the connection to the server.");
    }


}


// *************************** GAME MOVES ********************************** //
void BoardManager::startGame(){
    qDebug() << "Attempting to start a game.";

    // Build the json message
    QJsonObject data;
    data["action"] = "join_game";
    if(mode == "Online") {
        data["game_type"] = 0;
    }
    else if(mode == "Local") {
        data["game_type"] = 1;
    }
    else if(mode == "CPU") {
        data["game_type"] = 2;
    }

    // Send the message
    sendMessage(data);
}

void BoardManager::placePiece(uint8_t x, uint8_t y){
    qDebug() << "Attempting to place a piece.";

    QJsonObject data;
    data["action"] = "place_piece";
    data["x"] = x;
    data["y"] = y;

    sendMessage(data);
}

void BoardManager::removePiece(uint16_t pieceId){
    qDebug() << "Attempting to remove a piece.";

    QJsonObject data;
    data["action"] = "remove_piece";
    data["piece_ID"] = pieceId;

    sendMessage(data);
}

void BoardManager::movePiece(uint16_t pieceId, uint8_t x, uint8_t y){
    qDebug() << "Attempting to move a piece.";

    QJsonObject data;
    data["action"] = "move_piece";
    data["piece_ID"] = pieceId;
    data["new_x"] = x;
    data["new_y"] = y;

    sendMessage(data);
}

void BoardManager::quitGame(){
    qDebug() << "Attempting to end a game.";

    QJsonObject data;
    data["action"] = "quit_game";

    sendMessage(data);
}


// *************************** RESPONSE HANDLERS **************************** //
void BoardManager::setState(QString s){
    if(s == "STOPPED")
        gameState = GameState::STOPPED;
    else if(s == "FIRST_REMOVAL")
        gameState = GameState::FIRST_REMOVAL;
    else if(s == "REMOVAL")
        gameState = GameState::REMOVAL;
    else if(s == "PLACEMENT")
        gameState = GameState::PLACEMENT;
    else if(s == "MOVEMENT")
        gameState = GameState::MOVEMENT;
    else
        qDebug() << "Unknown game state: "<< s << "\n";
}

void BoardManager::startGameResponseHandler(QJsonObject &data){
    try {
        // Load the response values
        bool success = data["success"].toBool();
        QString error = data["error"].toString();
        bool isWaiting = data["waiting"].toBool();
        int num = data["player_num"].toInt();
        QString nextState = data["next_state"].toString();
        int nextPlayer = data["next_player"].toInt();

        // Load the adjacent pieces
        QHash<QPoint, QList<QPoint>> adjacentPieces;

        // Add each of the nodes to the adjacent pieces map
        const QJsonArray nodes = data["adjacent_pieces"].toArray();
        for (const auto& node: nodes) {

            // Create a list out of all the node's neighboring points
            QList<QPoint> neighborPoints;
            const QJsonArray neighbors = node.toObject()["neighbors"].toArray();
            for (const auto& neighbor: neighbors) {
                int x = neighbor.toObject()["x"].toInt();
                int y = neighbor.toObject()["y"].toInt();

                neighborPoints.append(QPoint(x, y));
            }

            // Add the node to the adjacentPieces map with its neighbors as its value
            int x = node.toObject()["x"].toInt();
            int y = node.toObject()["y"].toInt();
            adjacentPieces.insert(QPoint(x, y), neighborPoints);
        }

        // Check if the game started successfully
        if (success) {
            running = true;
            this->totalPieces[0] = 0;
            this->totalPieces[1] = 0;
        }

        // Update the game state
        setState(nextState);
        waiting = isWaiting;
        playerNum = num;
        currentTurn = nextPlayer;

        emit startGameResponded(success, error, isWaiting, nextState, nextPlayer, adjacentPieces);

    } catch (...) {
        qDebug() << "Received an unexpected response.\n";
    }
}

void BoardManager::placePieceResponseHandler(QJsonObject &data){
    try {
        // Move-independent parameters
        bool success = data["success"].toBool();
        QString error = data["error"].toString();
        int nextPlayer = data["next_player"].toInt();
        QString nextState = data["next_state"].toString();

        // Placement related parameters
        uint16_t newId = data["new_piece_ID"].toInt();
        uint8_t x = data["new_x"].toInt();
        uint8_t y = data["new_y"].toInt();

        QList<uint16_t> activePieces;
        for (const auto& piece: data["active_pieces"].toArray()) {
            activePieces.append(piece.toInt());
        }

        // Update the number of pieces
        if(success) {
            totalPieces[currentTurn] += 1;
        }

        // Notify the UI of the move's result
        emit placePieceResponded(success, error, newId, x, y, nextState, nextPlayer, activePieces);

        // Update the game state
        setState(nextState);
        currentTurn = nextPlayer;

    } catch (...) {
        qDebug() << "Received an unexpected response.\n";
    }

}

void BoardManager::removePieceResponseHandler(QJsonObject &data){
    try {
        // Move-independent parameters
        bool success = data["success"].toBool();
        QString error = data["error"].toString();
        int nextPlayer = data["next_player"].toInt();
        QString nextState = data["next_state"].toString();

        // Removal related parameters
        uint16_t removed_piece = data["removed_piece"].toInt();
        QList<uint16_t> activePieces;
        for (const auto& piece: data["active_pieces"].toArray()) {
            activePieces.append(piece.toInt());
        }

        // Update the number of pieces
        if(success) {
            totalPieces[(currentTurn + 1) % 2] -= 1;
        }

        // Notify the UI of the move's result
        emit removePieceResponded(success, error, removed_piece, nextState, nextPlayer, activePieces);

        // Update the game state
        setState(nextState);
        currentTurn = nextPlayer;

    } catch (...) {
        qDebug() << "Received an unexpected response.\n";
    }
}

void BoardManager::movePieceResponseHandler(QJsonObject &data){
    try {
        // Move-independent parameters
        bool success = data["success"].toBool();
        QString error = data["error"].toString();
        int nextPlayer = data["next_player"].toInt();
        QString nextState = data["next_state"].toString();

        // Movement related parameters
        uint16_t movedPiece = data["moved_piece"].toInt();
        uint8_t x = data["new_x"].toInt();
        uint8_t y = data["new_y"].toInt();
        QList<uint16_t> activePieces;
        for (const auto& piece: data["active_pieces"].toArray()) {
            activePieces.append(piece.toInt());
        }

        qDebug() << "Next state:" << nextState << ", Active pieces:" << activePieces;

        emit movePieceResponded(success, error, movedPiece, x, y, nextState, nextPlayer, activePieces);

        // Update the game state
        setState(nextState);
        currentTurn = nextPlayer;

    } catch (...) {
        qDebug() << "Received an unexpected response.\n";
    }
}

void BoardManager::quitGameResponseHandler(QJsonObject &data){
    try {
        bool success = data["success"].toBool();
        QString error = data["error"].toString();
        uint8_t winner = data["winner"].toInt();
        uint8_t flag = data["flag"][0].toInt();

        if (success){
            running = false;
            waiting = false;
            this->winner = winner;
        }

        emit quitGameResponded(success, error, winner, flag, waiting);

    } catch (...) {
        qDebug() << "Received an unexpected response.\n";
    }
}

