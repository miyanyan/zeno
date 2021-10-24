#include "qdmgraphicslinkhalf.h"
#include "qdmgraphicssocketin.h"
#include "qdmgraphicssocketout.h"
#include "qdmgraphicsscene.h"
#include <QGraphicsView>
#include <QCursor>

QDMGraphicsLinkHalf::QDMGraphicsLinkHalf(QDMGraphicsSocket *socket)
    : socket(socket)
{
    setZValue(1);
}

QPointF QDMGraphicsLinkHalf::getSrcPos() const {
    return dynamic_cast<QDMGraphicsSocketOut *>(socket) ? socket->getLinkedPos() : getMousePos();
}

QPointF QDMGraphicsLinkHalf::getDstPos() const {
    return dynamic_cast<QDMGraphicsSocketIn *>(socket) ? socket->getLinkedPos() : getMousePos();
}

QPointF QDMGraphicsLinkHalf::getMousePos() const {
    auto parentScene = static_cast<QDMGraphicsScene *>(scene());
    auto view = parentScene->views().at(0);
    auto res = view->mapToScene(view->mapFromGlobal(QCursor::pos()));
    return res;
}
