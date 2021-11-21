#ifndef QDMGRAPHICSSCENE_H
#define QDMGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <zeno/ztd/property.h>
#include "qdmgraphicsnode.h"
#include "qdmgraphicssocket.h"
#include "qdmgraphicslinkhalf.h"
#include "qdmgraphicslinkfull.h"
#include "qdmgraphicsbackground.h"
#include <QString>
#include <vector>
#include <set>

ZENO_NAMESPACE_BEGIN

struct Interceptor;

class QDMGraphicsNodeSubnet;
class QDMGraphicsNodeSubnetIn;
class QDMGraphicsNodeSubnetOut;

class QDMGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

    friend Interceptor;

    std::set<std::unique_ptr<QDMGraphicsNode>> nodes;
    std::set<std::unique_ptr<QDMGraphicsLinkFull>> links;
    // not null if the scene is a subnet:
    QDMGraphicsNodeSubnet *subnetNode{};
    QDMGraphicsNodeSubnetIn *subnetInNode{};
    QDMGraphicsNodeSubnetOut *subnetOutNode{};

    std::unique_ptr<QDMGraphicsBackground> background;
    std::unique_ptr<QDMGraphicsLinkHalf> pendingLink;
    QDMGraphicsNode *floatingNode{};
    QDMGraphicsNode *currentNode{};

    void addNode(QDMGraphicsNode *node);
    void addSubnetNode();
    void addNormalNode(std::string const &type);
    void setFloatingNode(QDMGraphicsNode *node);

public:
    QDMGraphicsScene();
    ~QDMGraphicsScene() override;

    QDMGraphicsLinkFull *addLink(QDMGraphicsSocket *srcSocket, QDMGraphicsSocket *dstSocket);
    void removeNode(QDMGraphicsNode *node);
    void removeLink(QDMGraphicsLinkFull *link);

    void initAsSubnet(QDMGraphicsNodeSubnet *node);
    void setCurrentNode(QDMGraphicsNode *node);
    //std::vector<QDMGraphicsNode *> getVisibleNodes() const;
    [[nodiscard]] std::vector<QDMGraphicsScene *> getChildScenes() const;
    [[nodiscard]] std::string allocateNodeName(std::string const &prefix) const;
    [[nodiscard]] std::string getFullPath() const;
    [[nodiscard]] std::string getName() const;

    [[nodiscard]] QPointF getCursorPos() const;
    void socketClicked(QDMGraphicsSocket *socket);
    void deletePressed();
    void copyPressed();
    void pastePressed();
    void blankClicked();
    void doubleClicked();
    void cursorMoved();

public slots:
    void updateSceneSelection();
    void addNodeByType(QString type);

signals:
    void sceneUpdated();
    void sceneCreatedOrRemoved();
    void currentNodeChanged(QDMGraphicsNode *node);
    void subnetSceneEntered(QDMGraphicsScene *subScene);
};

ZENO_NAMESPACE_END

#endif // QDMGRAPHICSSCENE_H
