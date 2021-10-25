#ifndef QDMGRAPHICSNODE_H
#define QDMGRAPHICSNODE_H

#include <QGraphicsItem>
#include <vector>
#include "qdmgraphicssocketin.h"
#include "qdmgraphicssocketout.h"
#include <QRectF>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <zeno/dop/Node.h>
#include <memory>

class QDMGraphicsNode : public QGraphicsItem
{
    std::vector<std::unique_ptr<QDMGraphicsSocketIn>> socketIns;
    std::vector<std::unique_ptr<QDMGraphicsSocketOut>> socketOuts;
    std::unique_ptr<QGraphicsTextItem> label;

    std::unique_ptr<zeno::dop::Node> dopNode;

public:
    QDMGraphicsNode();
    ~QDMGraphicsNode();

    void unlinkAll();
    float getHeight() const;
    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, QStyleOptionGraphicsItem const *styleOptions, QWidget *widget) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    QDMGraphicsSocketIn *addSocketIn();
    QDMGraphicsSocketOut *addSocketOut();
    void initByName(QString name);
    void setName(QString name);

    static constexpr float WIDTH = 200, HEIGHT = 60, ROUND = 6, BORDER = 3;
    static constexpr float SOCKMARGINTOP = 20, SOCKSTRIDE = 30, SOCKMARGINBOT = -10;
};

#endif // QDMGRAPHICSNODE_H
