#ifndef __MODEL_ACCEPTOR_H__
#define __MODEL_ACCEPTOR_H__

#include <zenoio/acceptor/iacceptor.h>

#include "modeldata.h"

class SubGraphModel;
class GraphsModel;

class ModelAcceptor : public IAcceptor
{
public:
	ModelAcceptor(GraphsModel* pModel, bool bImport);

	//IAcceptor
	bool setLegacyDescs(const rapidjson::Value& graphObj, const NODE_DESCS& nodesParams) override;
	void BeginSubgraph(const QString& name, int type) override;
	bool setCurrentSubGraph(IGraphsModel* pModel, const QModelIndex& subgIdx) override;
	void EndSubgraph() override;
    void EndGraphs() override;
	void setFilePath(const QString& fileName) override;
	void switchSubGraph(const QString& graphName) override;
	bool addNode(QString& nodeid, const QString& name, const QString& customName, const NODE_DESCS& descriptors) override;
	void setViewRect(const QRectF& rc) override;
	void setSocketKeys(const QString& id, const QStringList& keys) override;
	void initSockets(const QString& id, const QString& name, const NODE_DESCS& descs) override;
	void addDictKey(const QString& id, const QString& keyName, bool bInput) override;
	void addSocket(bool bInput, const QString& ident, const QString& sockName, const QString& sockProperty) override;

    void setInputSocket2(
        const QString& nodeCls,
        const QString& inNode,
        const QString& inSock,
        const QString& outLinkPath,
        const QString& sockProperty,
        const rapidjson::Value& defaultValue,
        const NODE_DESCS& legacyDescs
    ) override;

    void setInputSocket(
        const QString& nodeCls,
        const QString& inNode,
        const QString& inSock,
        const QString& outNode,
        const QString& outSock,
        const rapidjson::Value& defaultValue,
        const NODE_DESCS& legacyDescs
    ) override;

    void setOutputSocket(
        const QString& inNode,
        const QString& inSock,
        const QString& netlabel,
        const QString& type
    ) override;

    void addInnerDictKey(
        bool bInput,
        const QString& inNode,
        const QString& inSock,
        const QString& keyName,
        const QString& link,
        const QString& netLabel
    ) override;

    void setDictPanelProperty(
        bool bInput,
        const QString& ident,
        const QString& sockName,
        bool bCollasped
    ) override;

	void setControlAndProperties(const QString& nodeCls, const QString& inNode, const QString& inSock, PARAM_CONTROL control, const QVariant& ctrlProperties);
    void setToolTip(PARAM_CLASS cls, const QString &inNode, const QString &inSock, const QString &toolTip) override;
    void setNetLabel(PARAM_CLASS cls, const QString& inNode, const QString& inSock, const QString& netlabel) override;
	void setParamValue(const QString& id, const QString& nodeCls, const QString& name, const rapidjson::Value& value, const NODE_DESCS& legacyDescs) override;
    void setParamValue2(const QString &id, const QString &noCls, const PARAMS_INFO &params) override;
	void setPos(const QString& id, const QPointF& pos) override;
	void setOptions(const QString& id, const QStringList& options) override;
	void setColorRamps(const QString& id, const COLOR_RAMPS& colorRamps) override;
	void setBlackboard(const QString& id, const BLACKBOARD_INFO& blackboard) override;
	void setTimeInfo(const TIMELINE_INFO& info) override;
	void setRecordInfo(const RECORD_SETTING& info) override;
    void setLayoutInfo(const LAYOUT_SETTING& info) override;
    void setUserDataInfo(const USERDATA_SETTING& info) override;
	TIMELINE_INFO timeInfo() const override;
	RECORD_SETTING recordInfo() const override;
    LAYOUT_SETTING layoutInfo() const override;
    USERDATA_SETTING userdataInfo() const override;
	void setLegacyCurve(
        const QString& id,
        const QVector<QPointF>& pts,
        const QVector<QPair<QPointF, QPointF>>& hdls) override;
	QObject* currGraphObj() override;
	void endInputs(const QString& id, const QString& nodeCls) override;
	void endParams(const QString& id, const QString& nodeCls) override;
	void addCustomUI(const QString& id, const VPARAM_INFO& invisibleRoot) override;
    void setIOVersion(zenoio::ZSG_VERSION versio) override;
    void endNode(const QString& id, const QString& nodeCls, const rapidjson::Value& objValue) override;
    void addCommandParam(const rapidjson::Value& name, const QString& path) override;

private:
    void resolveAllLinks();

	TIMELINE_INFO m_timeInfo;
	RECORD_SETTING m_recordInfo;
	LAYOUT_SETTING m_layoutInfo;
    USERDATA_SETTING m_userdatInfo;
	QList<EdgeInfo> m_subgLinks;	//collected links for m_currentGraph.
    QList<EdgeInfo> m_subgLegacyLinks;  //links with legacy param.
	SubGraphModel* m_currentGraph;
	GraphsModel* m_pModel;
	bool m_bImport;
    QMap<QString, QString> m_oldToNewNodeIds; //<new id, old id>
};


#endif