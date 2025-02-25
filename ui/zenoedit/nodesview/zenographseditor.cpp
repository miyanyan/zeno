#include "zenographseditor.h"
#include "zenosubnetlistview.h"
#include <comctrl/ztoolbutton.h>
#include "zenoapplication.h"
#include "../nodesys/zenosubgraphscene.h"
#include "zenowelcomepage.h"
#include <zenomodel/include/graphsmanagment.h>
#include <zenomodel/include/modelrole.h>
#include <zenomodel/include/viewparammodel.h>
#include "variantptr.h"
#include <comctrl/zenocheckbutton.h>
#include <comctrl/ziconbutton.h>
#include <zenoui/style/zenostyle.h>
#include "zenomainwindow.h"
#include "ui_zenographseditor.h"
#include "nodesview/zsubnetlistitemdelegate.h"
#include "searchitemdelegate.h"
#include "common_def.h"
#include "startup/zstartup.h"
#include "util/log.h"
#include "settings/zsettings.h"
#include "dialog/zeditparamlayoutdlg.h"
#include <zenomodel/include/nodesmgr.h>
#include "settings/zenosettingsmanager.h"
#include <zenoui/comctrl/zpathedit.h>


ZenoGraphsEditor::ZenoGraphsEditor(ZenoMainWindow* pMainWin)
	: QWidget(nullptr)
	, m_mainWin(pMainWin)
    , m_model(nullptr)
    , m_searchOpts(SEARCHALL)
{
    initUI();
    initModel();
    initSignals();

    auto graphsMgm = zenoApp->graphsManagment();
    if (graphsMgm) {
        IGraphsModel* pModel = graphsMgm->currentModel();
        if (pModel) {
            resetModel(pModel);
        }
    }
}

ZenoGraphsEditor::~ZenoGraphsEditor()
{
}

void ZenoGraphsEditor::initUI()
{
	m_ui = new Ui::GraphsEditor;
	m_ui->setupUi(this);

    int _margin = ZenoStyle::dpiScaled(10);
    QMargins margins(_margin, _margin, _margin, _margin);
    QSize szIcons = ZenoStyle::dpiScaledSize(QSize(20, 20));
    m_ui->moreBtn->setIcons(szIcons, ":/icons/more.svg", ":/icons/more_on.svg");

    m_ui->splitter->setStretchFactor(1, 5);

    m_ui->mainStackedWidget->setCurrentWidget(m_ui->welcomeScrollPage);
    m_ui->stackedWidget->setCurrentIndex(0);

    QFont font = QApplication::font();
    font.setPointSize(10);
    m_ui->graphsViewTab->setFont(font);  //bug in qss font setting.
    m_ui->graphsViewTab->tabBar()->setDrawBase(false);
    m_ui->graphsViewTab->setIconSize(ZenoStyle::dpiScaledSize(QSize(20,20)));
    m_ui->searchEdit->setProperty("cssClass", "searchEditor");
    m_ui->graphsViewTab->setProperty("cssClass", "graphicsediter");
    m_ui->graphsViewTab->tabBar()->setProperty("cssClass", "graphicsediter");
    m_ui->graphsViewTab->setUsesScrollButtons(true);
    m_ui->btnSearchOpt->setIcons(":/icons/collaspe.svg", ":/icons/collaspe.svg");
    m_ui->btnSearchOpt->setToolTip(tr("Search Option"));
    ZIconLabel* pPageListButton = new ZIconLabel(m_ui->graphsViewTab);
    pPageListButton->setToolTip(tr("Tab List"));
    pPageListButton->setIcons(ZenoStyle::dpiScaledSize(QSize(20, 30)), ":/icons/ic_parameter_unfold.svg", ":/icons/ic_parameter_unfold.svg");
    connect(pPageListButton, &ZIconLabel::clicked, this, &ZenoGraphsEditor::onPageListClicked);
    m_ui->graphsViewTab->setCornerWidget(pPageListButton);
    m_ui->graphsViewTab->setDocumentMode(false);
    initRecentFiles();
}

void ZenoGraphsEditor::initModel()
{
    m_sideBarModel = new QStandardItemModel;

    QStandardItem* pItem = new QStandardItem;
	pItem->setData(Side_Subnet);
    m_sideBarModel->appendRow(pItem);

    pItem = new QStandardItem;
    pItem->setData(Side_Tree);
    m_sideBarModel->appendRow(pItem);

    pItem = new QStandardItem;
    pItem->setData(Side_Search);
    m_sideBarModel->appendRow(pItem);

    m_selection = new QItemSelectionModel(m_sideBarModel);
}

void ZenoGraphsEditor::initSignals()
{
	auto graphsMgr = zenoApp->graphsManagment();
	connect(&*graphsMgr, SIGNAL(modelInited(IGraphsModel*)), this, SLOT(resetModel(IGraphsModel*)));
    connect(graphsMgr->logModel(), &QStandardItemModel::rowsInserted, this, &ZenoGraphsEditor::onLogInserted);

    connect(m_selection, &QItemSelectionModel::selectionChanged, this, &ZenoGraphsEditor::onSideBtnToggleChanged);
    connect(m_selection, &QItemSelectionModel::currentChanged, this, &ZenoGraphsEditor::onCurrentChanged);

    //connect(m_ui->subnetList, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onListItemActivated(const QModelIndex&)));
    //connect(m_ui->subnetTree, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onTreeItemActivated(const QModelIndex&)));

	connect(m_ui->welcomePage, SIGNAL(newRequest()), m_mainWin, SLOT(onNewFile()));
	connect(m_ui->welcomePage, SIGNAL(openRequest()), m_mainWin, SLOT(openFileDialog()));

    connect(m_ui->moreBtn, SIGNAL(clicked()), this, SLOT(onSubnetOptionClicked()));
    connect(m_ui->btnSearchOpt, SIGNAL(clicked()), this, SLOT(onSearchOptionClicked()));
    connect(m_ui->graphsViewTab, &QTabWidget::tabCloseRequested, this, [=](int index) {
        m_ui->graphsViewTab->removeTab(index);
    });
    connect(m_ui->graphsViewTab, &QTabWidget::currentChanged, this, [=](int index) {
        if (m_ui->graphsViewTab->tabText(index).compare("main", Qt::CaseInsensitive) == 0)
        {
            closeMaterialTab();
        }
    });
    connect(m_ui->searchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(onSearchEdited(const QString&)));
    connect(m_ui->searchResView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(onSearchItemClicked(const QModelIndex&)));

    auto& inst = ZenoSettingsManager::GetInstance();
    if (!inst.getValue("zencache-enable").isValid())
    {
        ZenoSettingsManager::GetInstance().setValue("zencache-enable", true);
    }

    connect(&ZenoSettingsManager::GetInstance(), &ZenoSettingsManager::valueChanged, this, [=](QString zsName) {
        if (zsName == zsSubgraphType)
        {
            int type = ZenoSettingsManager::GetInstance().getValue(zsName).toInt();
            m_ui->label->setText(type == SUBGRAPH_TYPE::SUBGRAPH_NOR ? tr("Subnet") : type == SUBGRAPH_TYPE::SUBGRAPH_METERIAL ? tr("Material Subnet") : tr("Preset Subnet"));
            if (type != SUBGRAPH_METERIAL)
                closeMaterialTab();
        }
    });

    //m_selection->setCurrentIndex(m_sideBarModel->index(0, 0), QItemSelectionModel::SelectCurrent);
}

void ZenoGraphsEditor::initRecentFiles()
{
    m_ui->welcomePage->initRecentFiles();
}

void ZenoGraphsEditor::resetModel(IGraphsModel* pModel)
{
    if (!pModel)
    {
        onModelCleared();
        return;
    }

    auto mgr = zenoApp->graphsManagment();
    m_model = pModel;
    ZASSERT_EXIT(m_model);

    SubListSortProxyModel* treeProxyModel = new SubListSortProxyModel(this);
    treeProxyModel->setSourceModel(mgr->treeModel());
    treeProxyModel->setDynamicSortFilter(true);
    m_ui->subnetTree->setModel(treeProxyModel);
    treeProxyModel->sort(0, Qt::AscendingOrder);
    m_ui->subnetList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_ui->subnetTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ZenoGraphsEditor::onTreeItemSelectionChanged);

    SubListSortProxyModel* proxyModel = new SubListSortProxyModel(this);    
    proxyModel->setSourceModel(pModel);
    proxyModel->setDynamicSortFilter(true);
    m_ui->subnetList->setModel(proxyModel);
    proxyModel->sort(0, Qt::AscendingOrder);
    m_ui->subnetTree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ZSubnetListItemDelegate *delegate = new ZSubnetListItemDelegate(m_model, this);
    m_ui->subnetList->setItemDelegate(delegate);
    connect(m_ui->subnetList->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() {
        QModelIndexList lst = m_ui->subnetList->selectionModel()->selectedIndexes();
        delegate->setSelectedIndexs(lst);
        if (lst.size() == 1) 
        {
            onListItemActivated(lst.first());
        }
    });

    m_ui->mainStackedWidget->setCurrentWidget(m_ui->mainEditor);
    m_ui->graphsViewTab->clear();

    connect(pModel, &IGraphsModel::modelClear, this, &ZenoGraphsEditor::onModelCleared);
	connect(pModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)), this, SLOT(onSubGraphsToRemove(const QModelIndex&, int, int)));
	connect(pModel, SIGNAL(modelReset()), this, SLOT(onModelReset()));
	connect(pModel, SIGNAL(graphRenamed(const QString&, const QString&)), this, SLOT(onSubGraphRename(const QString&, const QString&)));    
    activateTab("main");
}

void ZenoGraphsEditor::onModelCleared()
{
    m_ui->mainStackedWidget->setCurrentWidget(m_ui->welcomeScrollPage);
    m_ui->searchEdit->clear();
}

void ZenoGraphsEditor::onSubGraphsToRemove(const QModelIndex& parent, int first, int last)
{
	for (int r = first; r <= last; r++)
	{
		QModelIndex subgIdx = m_model->index(r, 0);
		const QString& name = subgIdx.data(ROLE_OBJNAME).toString();
		int idx = tabIndexOfName(name);
		m_ui->graphsViewTab->removeTab(idx);
	}
}

void ZenoGraphsEditor::onModelReset()
{
	m_ui->graphsViewTab->clear();
    m_model = nullptr;
}

void ZenoGraphsEditor::onSubGraphRename(const QString& oldName, const QString& newName)
{
	int idx = tabIndexOfName(oldName);
	if (idx != -1)
	{
		QTabBar* pTabBar = m_ui->graphsViewTab->tabBar();
		pTabBar->setTabText(idx, newName);
	}
}

void ZenoGraphsEditor::onSearchOptionClicked()
{
	QMenu* pOptionsMenu = new QMenu;

	QAction* pNode = new QAction(tr("Node"));
    pNode->setCheckable(true);
    pNode->setChecked(m_searchOpts & SEARCH_NODECLS);

	QAction* pSubnet = new QAction(tr("Subnet"));
    pSubnet->setCheckable(true);
    pSubnet->setChecked(m_searchOpts & SEARCH_SUBNET);

	QAction* pAnnotation = new QAction(tr("Annotation"));
    pAnnotation->setCheckable(true);
    pAnnotation->setEnabled(false);

	QAction* pWrangle = new QAction(tr("Parameter"));
    pWrangle->setCheckable(true);
    pWrangle->setChecked(m_searchOpts & SEARCH_ARGS);

	pOptionsMenu->addAction(pNode);
	pOptionsMenu->addAction(pSubnet);
	pOptionsMenu->addAction(pAnnotation);
	pOptionsMenu->addAction(pWrangle);

	connect(pNode, &QAction::triggered, this, [=](bool bChecked) {
        if (bChecked)
            m_searchOpts |= SEARCH_NODECLS;
        else
            m_searchOpts &= (~(int)SEARCH_NODECLS);
	});

	connect(pSubnet, &QAction::triggered, this, [=](bool bChecked) {
		if (bChecked)
			m_searchOpts |= SEARCH_SUBNET;
		else
			m_searchOpts &= (~(int)SEARCH_SUBNET);
		});

	connect(pAnnotation, &QAction::triggered, this, [=](bool bChecked) {
		if (bChecked)
			m_searchOpts |= SEARCH_ANNO;
		else
			m_searchOpts &= (~(int)SEARCH_ANNO);
		});

	connect(pWrangle, &QAction::triggered, this, [=](bool bChecked) {
		if (bChecked)
			m_searchOpts |= SEARCH_ARGS;
		else
			m_searchOpts &= (~(int)SEARCH_ARGS);
		});

	pOptionsMenu->exec(QCursor::pos());
	pOptionsMenu->deleteLater();
}

void ZenoGraphsEditor::onNewSubgraph()
{
    bool bOk = false;
    QString newSubgName = QInputDialog::getText(this, tr("create subnet"), tr("new subgraph name:")
        , QLineEdit::Normal, "SubgraphName", &bOk);

    if (newSubgName.compare("main", Qt::CaseInsensitive) == 0)
    {
        QMessageBox msg(QMessageBox::Warning, tr("Zeno"), tr("main graph is not allowed to be created"));
        msg.exec();
        return;
    }

    if (bOk) {
        m_model->newSubgraph(newSubgName);
    }
}

void ZenoGraphsEditor::onPageListClicked()
{
    QMenu* pMenu = new QMenu;
    QSize size = ZenoStyle::dpiScaledSize(QSize(16, 16));
    for (int i = 0; i < m_ui->graphsViewTab->count(); i++)
    {
        QWidgetAction* pAction = new QWidgetAction(pMenu);
        QWidget* pWidget = new QWidget(pMenu);
        QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
        pLayout->setContentsMargins(0, ZenoStyle::dpiScaled(5), 0, ZenoStyle::dpiScaled(5));
        QString text = m_ui->graphsViewTab->tabText(i);
        ZToolButton* ptextButton = new ZToolButton(ZToolButton::Opt_TextRightToIcon, m_ui->graphsViewTab->tabIcon(i), size, text);
        ptextButton->setTextClr(QColor("#A3B1C0"), QColor("#FFFFFF"), QColor("#A3B1C0"), QColor("#FFFFFF"));
        pWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        pLayout->addWidget(ptextButton);
        pLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
        ZIconLabel* pDeleteButton = new ZIconLabel(pWidget);
        pDeleteButton->setToolTip(tr("Close Tab"));
        pDeleteButton->setIcons(size, ":/icons/closebtn.svg", ":/icons/closebtn_on.svg");
        pLayout->addWidget(pDeleteButton);
        pAction->setDefaultWidget(pWidget);
        pAction->setIcon(m_ui->graphsViewTab->tabIcon(i));
        pMenu->addAction(pAction);
        connect(pDeleteButton, &ZIconLabel::clicked, this, [=]()
        {
            int idx = tabIndexOfName(m_ui->graphsViewTab->tabText(i));
            if (idx >= 0)
                m_ui->graphsViewTab->removeTab(idx);
            pMenu->close();
        });
        connect(ptextButton, &ZToolButton::clicked, this, [=]() {
            const QModelIndex& index = m_model->index(text);
            if (index.isValid())
                activateTab(text, index.data(ROLE_OBJPATH).toString());
            pMenu->close();
        });

    }
    pMenu->exec(cursor().pos());
    pMenu->deleteLater();
}

void ZenoGraphsEditor::onSubnetOptionClicked()
{
    QMenu* pOptionsMenu = new QMenu;

    QAction* pNewSubg = new QAction(tr("create subnet"));
    QAction* pSubnetMap = new QAction(tr("subnet map"));
    QAction* pImpFromFile = new QAction(tr("import from local file"));
    QAction* pImpFromSys = new QAction(tr("import system subnet"));
    QAction* pImpFromPreset = new QAction(tr("import preset subnet"));

    pOptionsMenu->addAction(pNewSubg);
    pOptionsMenu->addAction(pSubnetMap);
    pOptionsMenu->addSeparator();
    pOptionsMenu->addAction(pImpFromFile);
    pOptionsMenu->addAction(pImpFromSys);
    pOptionsMenu->addAction(pImpFromPreset);

    connect(pNewSubg, &QAction::triggered, this, &ZenoGraphsEditor::onNewSubgraph);
    connect(pSubnetMap, &QAction::triggered, this, [=]() {

    });
    connect(pImpFromFile, &QAction::triggered, this, [=]() {
        m_mainWin->importGraph();
    });
    connect(pImpFromSys, &QAction::triggered, this, [=]() {

    });
    connect(pImpFromPreset, &QAction::triggered, this, [=]() {
        m_mainWin->importGraph(true);
    });

    pOptionsMenu->exec(QCursor::pos());
    pOptionsMenu->deleteLater();
}

void ZenoGraphsEditor::sideButtonToggled(bool bToggled)
{
    QObject* pBtn = sender();

    QModelIndex idx;
    /*
    if (pBtn == m_ui->subnetBtn)
    {
        idx = m_sideBarModel->match(m_sideBarModel->index(0, 0), Qt::UserRole + 1, Side_Subnet)[0];
    }
    else if (pBtn == m_ui->treeviewBtn)
    {
        idx = m_sideBarModel->match(m_sideBarModel->index(0, 0), Qt::UserRole + 1, Side_Tree)[0];
    }
    else if (pBtn == m_ui->searchBtn)
    {
        idx = m_sideBarModel->match(m_sideBarModel->index(0, 0), Qt::UserRole + 1, Side_Search)[0];
    }
    */

    if (bToggled)
        m_selection->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
    else
        m_selection->clearCurrentIndex();
}

void ZenoGraphsEditor::onSideBtnToggleChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
}

void ZenoGraphsEditor::onCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	if (previous.isValid())
	{
        int sideBar = current.data(Qt::UserRole + 1).toInt();
		switch (previous.data(Qt::UserRole + 1).toInt())
		{
        /*
		case Side_Subnet: m_ui->subnetBtn->setChecked(false); break;
		case Side_Tree: m_ui->treeviewBtn->setChecked(false); break;
		case Side_Search: m_ui->searchBtn->setChecked(false); break;
        */
		}
	}

	if (current.isValid())
	{
        m_ui->stackedWidget->show();
		int sideBar = current.data(Qt::UserRole + 1).toInt();
        switch (sideBar)
        {
            case Side_Subnet:
            {
                //m_ui->subnetBtn->setChecked(true);
                m_ui->stackedWidget->setCurrentWidget(m_ui->subnetPage);
                break;
            }
            case Side_Tree:
            {
                //m_ui->treeviewBtn->setChecked(true);
                m_ui->stackedWidget->setCurrentWidget(m_ui->treePage);
                break;
            }
            case Side_Search:
            {
                //m_ui->searchBtn->setChecked(true);
                m_ui->stackedWidget->setCurrentWidget(m_ui->searchPage);
                break;
            }
        }
	}
    else
    {
        m_ui->stackedWidget->hide();
    }
}

int ZenoGraphsEditor::tabIndexOfName(const QString& subGraphName)
{
	for (int i = 0; i < m_ui->graphsViewTab->count(); i++)
	{
		if (m_ui->graphsViewTab->tabText(i) == subGraphName)
		{
			return i;
		}
	}
	return -1;
}

void ZenoGraphsEditor::closeMaterialTab()
{
    for (int i = 0; i < m_ui->graphsViewTab->count(); i++)
    {
        QString subGraphName = m_ui->graphsViewTab->tabText(i);
        auto graphsMgm = zenoApp->graphsManagment();
        IGraphsModel* pModel = graphsMgm->currentModel();
        ZASSERT_EXIT(pModel);
        if (pModel->index(subGraphName).isValid())
        {
            if (pModel->index(subGraphName).data(ROLE_SUBGRAPH_TYPE).toInt() == SUBGRAPH_METERIAL)
            {
                m_ui->graphsViewTab->removeTab(i);
                break;
            }
        }
    }
}

void ZenoGraphsEditor::onListItemActivated(const QModelIndex& index)
{
	const QString& subgraphName = index.data().toString();
    activateTab(subgraphName);
}
void ZenoGraphsEditor::selectTab(const QString& subGraphName, const QString& path, std::vector<QString> & objIds)
{
    auto graphsMgm = zenoApp->graphsManagment();
    IGraphsModel* pModel = graphsMgm->currentModel();

    if (!pModel || !pModel->index(subGraphName).isValid())
        return;

    int idx = tabIndexOfName(subGraphName);
    if (idx == -1)
    {
        const QModelIndex& subgIdx = pModel->index(subGraphName);

        ZenoSubGraphScene* pScene = qobject_cast<ZenoSubGraphScene*>(graphsMgm->gvScene(subgIdx));
        if (!pScene)
        {
            pScene = new ZenoSubGraphScene(graphsMgm);
            graphsMgm->addScene(subgIdx, pScene);
            pScene->initModel(subgIdx);
        }

        pScene->select(objIds);
    }
    const QModelIndex& subgIdx = pModel->index(subGraphName);
    ZenoSubGraphScene* pScene = qobject_cast<ZenoSubGraphScene*>(graphsMgm->gvScene(subgIdx));
    if (!pScene)
    {
        pScene = new ZenoSubGraphScene(graphsMgm);
        graphsMgm->addScene(subgIdx, pScene);
        pScene->initModel(subgIdx);
    }

    pScene->select(objIds);
}

ZenoSubGraphView* ZenoGraphsEditor::getCurrentSubGraphView()
{
    if (ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget()))
    {
        return pView;
    }
    return nullptr;
}

void ZenoGraphsEditor::activateTab(const QString& subGraphName, const QString& path, const QString& objId, bool isError)
{
	auto graphsMgm = zenoApp->graphsManagment();
	IGraphsModel* pModel = graphsMgm->currentModel();

    if (!pModel->index(subGraphName).isValid())
        return;
    if (subGraphName.compare("main", Qt::CaseInsensitive) == 0)
    {
        closeMaterialTab();
    }
	int idx = tabIndexOfName(subGraphName);
	if (idx == -1)
	{
		const QModelIndex& subgIdx = pModel->index(subGraphName);
        if (subgIdx.data(ROLE_SUBGRAPH_TYPE).toInt() == SUBGRAPH_METERIAL)
        {
            closeMaterialTab();
        }

            ZenoSubGraphScene* pScene = qobject_cast<ZenoSubGraphScene*>(graphsMgm->gvScene(subgIdx));
            if (!pScene)
            {
                pScene = new ZenoSubGraphScene(graphsMgm);
                graphsMgm->addScene(subgIdx, pScene);
                pScene->initModel(subgIdx);
            }

            ZenoSubGraphView* pView = new ZenoSubGraphView;
            connect(pView, &ZenoSubGraphView::zoomed, pScene, &ZenoSubGraphScene::onZoomed);
            connect(pView, &ZenoSubGraphView::zoomed, this, &ZenoGraphsEditor::zoomed);
            pView->initScene(pScene);

            idx = m_ui->graphsViewTab->addTab(pView, subGraphName);

            QString tabIcon;
            if (subGraphName.compare("main", Qt::CaseInsensitive) == 0)
                tabIcon = ":/icons/subnet-main.svg";
            else
                tabIcon = ":/icons/subnet-general.svg";
            m_ui->graphsViewTab->setTabIcon(idx, QIcon(tabIcon));

            connect(pView, &ZenoSubGraphView::pathUpdated, this, [=](QString newPath) {
                QStringList L = newPath.split("/", QtSkipEmptyParts);
                QString subgName = L.last();
                activateTab(subgName, newPath);
            });
	}
	m_ui->graphsViewTab->setCurrentIndex(idx);

    ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
    ZASSERT_EXIT(pView);
    pView->resetPath(path, subGraphName, objId, isError);

    m_mainWin->onNodesSelected(pModel->index(subGraphName), pView->scene()->selectNodesIndice(), true);
}

void ZenoGraphsEditor::showFloatPanel(const QModelIndex &subgIdx, const QModelIndexList &nodes) {
    ZenoSubGraphView *pView = qobject_cast<ZenoSubGraphView *>(m_ui->graphsViewTab->currentWidget());
    if (pView != NULL)
    {
        pView->showFloatPanel(subgIdx, nodes);
    }
}

void ZenoGraphsEditor::onTreeItemActivated(const QModelIndex& index)
{
	QModelIndex idx = index;

	const QString& objId = idx.data(ROLE_OBJID).toString();
	QString path, subgName;
	if (!idx.parent().isValid())
	{
        subgName = idx.data(ROLE_OBJNAME).toString();
		path = "/" + subgName;
	}
	else
	{
		idx = idx.parent();
        subgName = idx.data(ROLE_OBJNAME).toString();

		while (idx.isValid())
		{
			QString objName = idx.data(ROLE_OBJNAME).toString();
			path = "/" + objName + path;
			idx = idx.parent();
		}
	}

    activateTab(subgName, path, objId);
}

void ZenoGraphsEditor::onPageActivated(const QPersistentModelIndex& subgIdx, const QPersistentModelIndex& nodeIdx)
{
    const QString& subgName = nodeIdx.data(ROLE_OBJNAME).toString();
    activateTab(subgName);
}

void ZenoGraphsEditor::onLogInserted(const QModelIndex& parent, int first, int last)
{
    if (!m_model)
        return;
    QStandardItemModel* logModel = qobject_cast<QStandardItemModel*>(sender());
    const QModelIndex& idx = logModel->index(first, 0, parent);
    if (idx.isValid())
    {
        QString objId = idx.data(ROLE_NODE_IDENT).toString();
        const QString& msg = idx.data(Qt::DisplayRole).toString();
        QtMsgType type = (QtMsgType)idx.data(ROLE_LOGTYPE).toInt();
        if (!objId.isEmpty() && type == QtFatalMsg)
        {
            if (objId.indexOf('/') != -1)
            {
                auto lst = objId.split('/', QtSkipEmptyParts);
                objId = lst.last();
                lst.removeOne(objId);
                markSubgError(lst);
            }

            QList<SEARCH_RESULT> results = m_model->search(objId, SEARCH_NODEID, SEARCH_MATCH_EXACTLY);
            for (int i = 0; i < results.length(); i++)
            {
                const SEARCH_RESULT& res = results[i];
                const QString &subgName = res.subgIdx.data(ROLE_OBJNAME).toString();

                QVariant varFocusOnError = ZenoSettingsManager::GetInstance().getValue(zsTraceErrorNode);

                bool bFocusOnError = true;
                if (varFocusOnError.type() == QVariant::Bool) {
                    bFocusOnError = varFocusOnError.toBool();
                }
                if (bFocusOnError)
                {
                    activateTab(subgName, "", objId, true);
                    if (i == results.length() - 1)
                        break;

                    QMessageBox msgbox(QMessageBox::Question, "", tr("next one?"), QMessageBox::Yes | QMessageBox::No);
                    int ret = msgbox.exec();
                    if (ret & QMessageBox::Yes) {
                    }
                    else {
                        break;
                    }
                }
                else
                {
                    const QModelIndex& subgIdx = m_model->index(subgName);
                    auto graphsMgm = zenoApp->graphsManagment();
                    ZenoSubGraphScene* pScene = qobject_cast<ZenoSubGraphScene*>(graphsMgm->gvScene(subgIdx));
                    if (!pScene) {
                        pScene = new ZenoSubGraphScene(graphsMgm);
                        graphsMgm->addScene(subgIdx, pScene);
                        pScene->initModel(subgIdx);
                    }
                    pScene->markError(objId);
                }
            }
        }
    }
}

void ZenoGraphsEditor::markSubgError(const QStringList& idents)
{
    for (const auto &ident : idents)
    {
        QModelIndex index = m_model->nodeIndex(ident);
        ZASSERT_EXIT(index.isValid());
        QModelIndex subgIdx = index.data(ROLE_SUBGRAPH_IDX).toModelIndex();
        ZASSERT_EXIT(subgIdx.isValid());
        auto graphsMgm = zenoApp->graphsManagment();
        ZenoSubGraphScene* pScene = qobject_cast<ZenoSubGraphScene*>(graphsMgm->gvScene(subgIdx));
        if (!pScene) {
            pScene = new ZenoSubGraphScene(graphsMgm);
            graphsMgm->addScene(subgIdx, pScene);
            pScene->initModel(subgIdx);
        }
        pScene->markError(ident);
    }
}

void ZenoGraphsEditor::onSearchEdited(const QString& content)
{
    QList<SEARCH_RESULT> results = m_model->search(content, m_searchOpts, SEARCH_FUZZ);

    QStandardItemModel* pModel = new QStandardItemModel(this);

    for (SEARCH_RESULT res : results)
    {
        if (res.type == SEARCH_SUBNET)
        {
            QString subgName = res.targetIdx.data(ROLE_OBJNAME).toString();
            QModelIndexList lst = pModel->match(pModel->index(0, 0), ROLE_OBJNAME, subgName, 1, Qt::MatchExactly);
            if (lst.size() == 0)
            {
                //add subnet
                QStandardItem* pItem = new QStandardItem(subgName + " (Subnet)");
                pItem->setData(subgName, ROLE_OBJNAME);
                pItem->setData(res.targetIdx.data(ROLE_OBJID).toString(), ROLE_OBJID);
                pModel->appendRow(pItem);
            }
        }
        else if (res.type == SEARCH_NODECLS || res.type == SEARCH_NODEID || res.type == SEARCH_ARGS || res.type == SEARCH_CUSTOM_NAME)
        {
            QString subgName = res.subgIdx.data(ROLE_OBJNAME).toString();
            QModelIndexList lst = pModel->match(pModel->index(0, 0), ROLE_OBJNAME, subgName, 1, Qt::MatchExactly);

            QStandardItem* parentItem = nullptr;
            if (lst.size() == 0)
            {
                //add subnet
                parentItem = new QStandardItem(subgName + " (Subnet)");
                parentItem->setData(subgName, ROLE_OBJNAME);
                pModel->appendRow(parentItem);
            }
            else
            {
                ZASSERT_EXIT(lst.size() == 1);
                parentItem = pModel->itemFromIndex(lst[0]);
            }

            QString nodeName = res.targetIdx.data(ROLE_OBJNAME).toString();
            QString nodeIdent = res.targetIdx.data(ROLE_OBJID).toString();
            QStandardItem* pItem = new QStandardItem(nodeIdent);
            pItem->setData(nodeName, ROLE_OBJNAME);
            pItem->setData(res.targetIdx.data(ROLE_OBJID).toString(), ROLE_OBJID);
            parentItem->appendRow(pItem);
        }
    }

    m_ui->searchResView->setModel(pModel);
    m_ui->searchResView->setItemDelegate(new SearchItemDelegate(content));
    m_ui->searchResView->expandAll();
}

void ZenoGraphsEditor::onSearchItemClicked(const QModelIndex& index)
{
    QString objId = index.data(ROLE_OBJID).toString();
    if (index.parent().isValid())
    {
        QString parentId = index.parent().data(ROLE_OBJID).toString();
        QString subgName = index.parent().data(ROLE_OBJNAME).toString();
        activateTab(subgName, "", objId);
    }
    else
    {
        QString subgName = index.data(ROLE_OBJNAME).toString();
        activateTab(subgName);
    }
}

void ZenoGraphsEditor::toggleViewForSelected(bool bOn)
{
    ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
    if (pView)
    {
        ZenoSubGraphScene* pScene = pView->scene();
        QModelIndexList nodes = pScene->selectNodesIndice();
        const QModelIndex& subgIdx = pScene->subGraphIndex();
        for (QModelIndex idx : nodes)
        {
            STATUS_UPDATE_INFO info;
            int options = idx.data(ROLE_OPTIONS).toInt();
            info.oldValue = options;
            if (bOn) {
                options |= OPT_VIEW;
            }
            else {
                options &= (~OPT_VIEW);
            }
            info.role = ROLE_OPTIONS;
            info.newValue = options;
            m_model->updateNodeStatus(idx.data(ROLE_OBJID).toString(), info, subgIdx);
        }
    }
}

void ZenoGraphsEditor::onSubnetListPanel(bool bShow, SideBarItem item) 
{
    QModelIndex idx = m_sideBarModel->match(m_sideBarModel->index(0, 0), Qt::UserRole + 1, item)[0];
    m_selection->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
    m_ui->stackedWidget->setVisible(bShow);
}

void ZenoGraphsEditor::onMenuActionTriggered(QAction* pAction)
{
    onAction(pAction);
}

void ZenoGraphsEditor::onCommandDispatched(QAction* pAction, bool bTriggered)
{
    onAction(pAction);
}

void ZenoGraphsEditor::onTreeItemSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) 
{
    QModelIndexList lst = m_ui->subnetTree->selectionModel()->selectedIndexes();
    if (lst.isEmpty())
        return;

    QModelIndex idx = lst.first();
    if (lst.size() == 1) 
    {
        onTreeItemActivated(idx);
    } 
    else 
    {
        QModelIndexList indexs;
        if (!idx.parent().isValid())
            return;
        QString parentName = idx.parent().data(ROLE_OBJNAME).toString();
        indexs << idx;
        for (auto index : lst) 
        {
            if (index == idx)
                continue;
            if (index.parent().data(ROLE_OBJNAME).toString() == parentName) {
                indexs << index;
            }
        }
        ZenoSubGraphView *pView = qobject_cast<ZenoSubGraphView *>(m_ui->graphsViewTab->currentWidget());
        ZASSERT_EXIT(pView);
        pView->selectNodes(indexs);
    }
}

void ZenoGraphsEditor::onAction(QAction* pAction, const QVariantList& args, bool bChecked)
{
    int actionType = pAction->property("ActionType").toInt();
    if (actionType == ZenoMainWindow::ACTION_COLLASPE)
    {
        ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        ZASSERT_EXIT(pView);
        QModelIndex subgIdx = pView->scene()->subGraphIndex();
        m_model->collaspe(subgIdx);
    }
    else if (actionType == ZenoMainWindow::ACTION_EXPAND) 
	{
		ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        ZASSERT_EXIT(pView);
		QModelIndex subgIdx = pView->scene()->subGraphIndex();
		m_model->expand(subgIdx);
    }
    else if (actionType == ZenoMainWindow::ACTION_OPEN_VIEW) 
    {
        toggleViewForSelected(true);
    }
    else if (actionType == ZenoMainWindow::ACTION_CLEAR_VIEW) 
    {
        toggleViewForSelected(false);
    }    
    else if (actionType == ZenoMainWindow::ACTION_CUSTOM_UI) 
    {
        ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        if (pView)
        {
            ZenoSubGraphScene* pScene = pView->scene();
            QModelIndexList nodes = pScene->selectNodesIndice();
            if (nodes.size() == 1)
            {
                QModelIndex nodeIdx = nodes[0];
                //only subgraph node
                if (!m_model->IsSubGraphNode(nodeIdx)) 
                {
                    QMessageBox::information(this, tr("Info"), tr("Cannot edit parameters!"));
                    return;
                }
                QStandardItemModel* viewParams = QVariantPtr<QStandardItemModel>::asPtr(nodeIdx.data(ROLE_NODE_PARAMS));
                ZASSERT_EXIT(viewParams);
                ZEditParamLayoutDlg dlg(viewParams, true, nodeIdx, m_model, this);
                dlg.exec();
            }
        }
    } 
    else if (actionType == ZenoMainWindow::ACTION_GROUP) 
    {
        ZenoSubGraphView *pView = qobject_cast<ZenoSubGraphView *>(m_ui->graphsViewTab->currentWidget());
        if (pView) 
        {
            ZenoSubGraphScene *pScene = pView->scene();
            NodesMgr::createNewNode(m_model, pScene->subGraphIndex(), "Group", QPointF());
        }
    }
    else if (actionType == ZenoMainWindow::ACTION_EASY_GRAPH) 
    {
        ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        if (pView)
        {
            ZenoSubGraphScene* pScene = pView->scene();
            QModelIndexList nodes = pScene->selectNodesIndice();
            QModelIndexList links = pScene->selectLinkIndice();
            bool bOk = false;
            QString newSubgName = QInputDialog::getText(this, tr("create subnet"), tr("new subgraph name:") , QLineEdit::Normal, "subgraph name", &bOk);
            if (bOk)
            {
                QModelIndex fromSubgIdx = pView->scene()->subGraphIndex();
                QModelIndex toSubgIdx = m_model->extractSubGraph(nodes, links, fromSubgIdx, newSubgName, true);
                if (toSubgIdx.isValid())
                {
                    activateTab(toSubgIdx.data(ROLE_OBJNAME).toString());
                }
            }
            else
            {
                //todo: msg to feedback.
            }
        }
    }
    else if (actionType == ZenoMainWindow::ACTION_SET_NASLOC) 
    {
        QSettings settings(zsCompanyName, zsEditor);
        QString v = settings.value("nas_loc").toString();

        QDialog dlg(this);
        QGridLayout *pLayout = new QGridLayout(&dlg);
        QDialogButtonBox *pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        CALLBACK_SWITCH cbSwitch = [=](bool bOn) {
            zenoApp->getMainWindow()->setInDlgEventLoop(bOn); //deal with ubuntu dialog slow problem when update viewport.
        };
        ZPathEdit *pathLineEdit = new ZPathEdit(cbSwitch, v, &dlg);
        pLayout->addWidget(new QLabel("Set NASLOC"), 2, 0);
        pLayout->addWidget(pathLineEdit, 2, 1);
        pLayout->addWidget(pButtonBox, 4, 1);
        connect(pButtonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
        connect(pButtonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));
        if (QDialog::Accepted == dlg.exec()) {
            QString text = pathLineEdit->text();
            text.replace('\\', '/');
            settings.setValue("nas_loc", text);
            // refresh settings (zeno::setConfigVariable), only needed in single-process mode
            startUp(true);
        }
    }
    else if (actionType == ZenoMainWindow::ACTION_ZENCACHE) 
    {
        QSettings settings(zsCompanyName, zsEditor);

        auto &inst = ZenoSettingsManager::GetInstance();

        QVariant varEnableCache = inst.getValue("zencache-enable");
        QVariant varTempCacheDir = inst.getValue("zencache-autoremove");
        QVariant varCacheRoot = inst.getValue("zencachedir");
        QVariant varCacheNum = inst.getValue("zencachenum");
        QVariant varAutoCleanCache = inst.getValue("zencache-autoclean");

        bool bEnableCache = varEnableCache.isValid() ? varEnableCache.toBool() : false;
        bool bTempCacheDir = varTempCacheDir.isValid() ? varTempCacheDir.toBool() : false;
        QString cacheRootDir = varCacheRoot.isValid() ? varCacheRoot.toString() : "";
        int cacheNum = varCacheNum.isValid() ? varCacheNum.toInt() : 1;
        bool bAutoCleanCache = varAutoCleanCache.isValid() ? varAutoCleanCache.toBool() : true;

        CALLBACK_SWITCH cbSwitch = [=](bool bOn) {
            zenoApp->getMainWindow()->setInDlgEventLoop(bOn); //deal with ubuntu dialog slow problem when update viewport.
        };
        ZPathEdit *pathLineEdit = new ZPathEdit(cbSwitch, cacheRootDir);
        pathLineEdit->setFixedWidth(256);
        pathLineEdit->setEnabled(!bTempCacheDir && bEnableCache);
        QCheckBox *pTempCacheDir = new QCheckBox;
        pTempCacheDir->setCheckState(bTempCacheDir ? Qt::Checked : Qt::Unchecked);
        pTempCacheDir->setEnabled(bEnableCache);
        QCheckBox* pAutoCleanCache = new QCheckBox;
        pAutoCleanCache->setCheckState(bAutoCleanCache ? Qt::Checked : Qt::Unchecked);
        pAutoCleanCache->setEnabled(bEnableCache && !bTempCacheDir);
        connect(pTempCacheDir, &QCheckBox::stateChanged, [=](bool state) {
            pathLineEdit->setText("");
            pathLineEdit->setEnabled(!state);
            pAutoCleanCache->setChecked(Qt::Unchecked);
            pAutoCleanCache->setEnabled(!state);
            });

        QSpinBox* pSpinBox = new QSpinBox;
        pSpinBox->setRange(1, 10000);
        pSpinBox->setValue(cacheNum);
        pSpinBox->setEnabled(bEnableCache);

        QCheckBox *pCheckbox = new QCheckBox;
        pCheckbox->setCheckState(bEnableCache ? Qt::Checked : Qt::Unchecked);
        connect(pCheckbox, &QCheckBox::stateChanged, [=](bool state) {
            if (!state)
            {
                pSpinBox->clear();
                pathLineEdit->clear();
                pTempCacheDir->setCheckState(Qt::Unchecked);
                pAutoCleanCache->setCheckState(Qt::Unchecked);
            }
            pSpinBox->setEnabled(state);
            pathLineEdit->setEnabled(state);
            pTempCacheDir->setEnabled(state);
            pAutoCleanCache->setEnabled(state && !pTempCacheDir->isChecked());
        });

        QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        QDialog dlg(this);
        QGridLayout* pLayout = new QGridLayout;
        pLayout->addWidget(new QLabel(tr("Enable cache")), 0, 0);
        pLayout->addWidget(pCheckbox, 0, 1);
        pLayout->addWidget(new QLabel(tr("Cache num")), 1, 0);
        pLayout->addWidget(pSpinBox, 1, 1);
        pLayout->addWidget(new QLabel(tr("Temp cache directory")), 2, 0);
        pLayout->addWidget(pTempCacheDir, 2, 1);
        pLayout->addWidget(new QLabel(tr("Cache root")), 3, 0);
        pLayout->addWidget(pathLineEdit, 3, 1);
        pLayout->addWidget(new QLabel(tr("Cache auto clean up")), 4, 0);
        pLayout->addWidget(pAutoCleanCache, 4, 1);
        pLayout->addWidget(pButtonBox, 5, 1);

        connect(pButtonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
        connect(pButtonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));

        dlg.setLayout(pLayout);
        if (QDialog::Accepted == dlg.exec())
        {
            inst.setValue("zencache-enable", pCheckbox->checkState() == Qt::Checked);
            inst.setValue("zencache-autoremove", pTempCacheDir->checkState() == Qt::Checked);
            inst.setValue("zencachedir", pathLineEdit->text());
            inst.setValue("zencachenum", pSpinBox->value());
            inst.setValue("zencache-autoclean", pAutoCleanCache->checkState() == Qt::Checked);
        }
    }
    else if (actionType == ZenoMainWindow::ACTION_ZOOM) 
    {
        ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        if (pView)
        {
            if (!args.isEmpty() && (args[0].type() == QMetaType::Float || args[0].type() == QMetaType::Double)) {
                pView->setZoom(args[0].toFloat());
            }
        }
    }
    else if (actionType == ZenoMainWindow::ACTION_UNDO) 
    {
        IGraphsModel* pGraphsModel = zenoApp->graphsManagment()->currentModel();
        if (pGraphsModel)
            pGraphsModel->undo();
    }
    else if (actionType == ZenoMainWindow::ACTION_REDO) 
    {
        IGraphsModel* pGraphsModel = zenoApp->graphsManagment()->currentModel();
        if (pGraphsModel)
            pGraphsModel->redo();
    }
    else if (actionType == ZenoMainWindow::ACTION_SELECT_NODE) 
    {
        ZenoSubGraphView* pView = qobject_cast<ZenoSubGraphView*>(m_ui->graphsViewTab->currentWidget());
        QModelIndex nodeIdx = pAction->data().toModelIndex();
        if (pView && nodeIdx.isValid())
            pView->focusOn(nodeIdx.data(ROLE_OBJID).toString());
    }
    else if (actionType == ZenoMainWindow::ACTION_NEW_SUBGRAPH)
    {
        onNewSubgraph();
    }
}
