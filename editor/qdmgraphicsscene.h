#ifndef QDMGRAPHICSSCENE_H
#define QDMGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <set>
#include "qdmgraphicsnode.h"
#include "qdmgraphicssocket.h"
#include "qdmgraphicslinkhalf.h"
#include "qdmgraphicslinkfull.h"
#include <QString>

class QDMGraphicsScene : public QGraphicsScene
{
    std::set<QDMGraphicsNode *> nodes;
    std::set<QDMGraphicsLinkFull *> links;
    QDMGraphicsLinkHalf *pendingLink{nullptr};

public:
    QDMGraphicsScene();
    ~QDMGraphicsScene();

    QDMGraphicsNode *addNode();
    QDMGraphicsLinkFull *addLink(QDMGraphicsSocket *srcSocket, QDMGraphicsSocket *dstSocket);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void removeLink(QDMGraphicsLinkFull *link);
    void socketClicked(QDMGraphicsSocket *socket);
    void blankClicked();
    void cursorMoved();

public slots:
    QDMGraphicsNode *addNodeByName(QString name);
};

#endif // QDMGRAPHICSSCENE_H
