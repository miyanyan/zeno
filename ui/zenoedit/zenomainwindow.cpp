#include "zenomainwindow.h"
#include "dock/zenodockwidget.h"
#include "graphsmanagment.h"
#include "launch/corelaunch.h"
#include "launch/serialize.h"
#include "model/graphsmodel.h"
#include "nodesview/zenographseditor.h"
#include "panel/zenodatapanel.h"
#include "panel/zenoproppanel.h"
#include "panel/zlogpanel.h"
#include "timeline/ztimeline.h"
#include "tmpwidgets/ztoolbar.h"
#include "viewport/viewportwidget.h"
#include "viewport/zenovis.h"
#include "zenoapplication.h"
#include <zeno/utils/log.h>
#include <zenoio/reader/zsgreader.h>
#include <zenoio/writer/zsgwriter.h>
#include <zenoui/model/modeldata.h>
#include <zenoui/style/zenostyle.h>
#include <zenoui/util/uihelper.h>


ZenoMainWindow::ZenoMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_pEditor(nullptr)
    , m_viewDock(nullptr)
    , m_bInDlgEventloop(false)
    , m_logger(nullptr)
{
    init();
    setContextMenuPolicy(Qt::NoContextMenu);
    setWindowTitle("Zeno Editor (github.com/zenustech/zeno)");
#ifdef __linux__
    if (char *p = std::getenv("ZENO_OPEN")) {
        printf("ZENO_OPEN: %s\n", p);
        openFile(p);
    }
#endif
}

ZenoMainWindow::~ZenoMainWindow()
{
}

void ZenoMainWindow::init()
{
    initMenu();
    initDocks();
    verticalLayout();
    //onlyEditorLayout();

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(11, 11, 11));
    setAutoFillBackground(true);
    setPalette(pal);
}

void ZenoMainWindow::initMenu() {
    QMenuBar *pMenuBar = new QMenuBar(this);
    if (!pMenuBar)
        return;

    QMenu *pFile = new QMenu(tr("File"));
    {
        QAction *pAction = new QAction(tr("New"), pFile);
        QMenu *pNewMenu = new QMenu;
        QAction *pNewGraph = pNewMenu->addAction("New Scene");
        connect(pNewGraph, SIGNAL(triggered()), this, SLOT(onNewFile()));

        pAction->setMenu(pNewMenu);

        pFile->addAction(pAction);

        pAction = new QAction(tr("Open"), pFile);
        pAction->setCheckable(false);
        pAction->setShortcut(QKeySequence(tr("Ctrl+O")));
        connect(pAction, SIGNAL(triggered()), this, SLOT(openFileDialog()));
        pFile->addAction(pAction);

        pAction = new QAction(tr("Save"), pFile);
        pAction->setCheckable(false);
        pAction->setShortcut(QKeySequence(tr("Ctrl+S")));
        connect(pAction, SIGNAL(triggered()), this, SLOT(save()));
        pFile->addAction(pAction);

        pAction = new QAction(tr("Save As"), pFile);
        pAction->setCheckable(false);
        connect(pAction, SIGNAL(triggered()), this, SLOT(saveAs()));
        pFile->addAction(pAction);

        pAction = new QAction(tr("Import"), pFile);
        pAction->setCheckable(false);
        pAction->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
        connect(pAction, SIGNAL(triggered()), this, SLOT(importGraph()));
        pFile->addAction(pAction);

        pAction = new QAction(tr("Export"), pFile);
        pAction->setCheckable(false);
        pAction->setShortcut(QKeySequence(tr("Ctrl+Shift+E")));
        connect(pAction, SIGNAL(triggered()), this, SLOT(exportGraph()));
        pFile->addAction(pAction);

        pAction = new QAction(tr("Close"), pFile);
        connect(pAction, SIGNAL(triggered()), this, SLOT(saveQuit()));
        pFile->addAction(pAction);
    }

    QMenu *pEdit = new QMenu(tr("Edit"));
    {
        QAction *pAction = new QAction(tr("Undo"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Redo"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Cut"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Copy"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Paste"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);
    }

    QMenu *pRender = new QMenu(tr("Render"));

    QMenu *pView = new QMenu(tr("View"));
    {
        QAction *pViewAct = new QAction(tr("view"));
        pViewAct->setCheckable(true);
        pViewAct->setChecked(true);
        connect(pViewAct, &QAction::triggered, this, [=]() { onToggleDockWidget(DOCK_VIEW, pViewAct->isChecked()); });
        pView->addAction(pViewAct);

        QAction* pEditorAct = new QAction(tr("editor"));
        pEditorAct->setCheckable(true);
        pEditorAct->setChecked(true);
        connect(pEditorAct, &QAction::triggered, this, [=]() { onToggleDockWidget(DOCK_EDITOR, pEditorAct->isChecked()); });
        pView->addAction(pEditorAct);

        QAction* pPropAct = new QAction(tr("property"));
        pPropAct->setCheckable(true);
        pPropAct->setChecked(true);
        connect(pPropAct, &QAction::triggered, this, [=]() { onToggleDockWidget(DOCK_NODE_PARAMS, pPropAct->isChecked()); });
        pView->addAction(pPropAct);

        QAction* pLoggerAct = new QAction(tr("logger"));
        pLoggerAct->setCheckable(true);
        pLoggerAct->setChecked(true);
        connect(pLoggerAct, &QAction::triggered, this, [=]() { onToggleDockWidget(DOCK_LOG, pLoggerAct->isChecked()); });
        pView->addAction(pLoggerAct);

        connect(pView, &QMenu::aboutToShow, this, [=]() {
            auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
            for (ZenoDockWidget *dock : docks)
            {
                DOCK_TYPE type = dock->type();
                switch (type)
                {
                    case DOCK_VIEW:     pViewAct->setChecked(dock->isVisible());    break;
                    case DOCK_EDITOR:   pEditorAct->setChecked(dock->isVisible());  break;
                    case DOCK_NODE_PARAMS: pPropAct->setChecked(dock->isVisible()); break;
                    case DOCK_LOG:      pLoggerAct->setChecked(dock->isVisible()); break;
                }
            }
        });
    }

    QMenu *pWindow = new QMenu(tr("Window"));

    QMenu *pHelp = new QMenu(tr("Help"));

    pMenuBar->addMenu(pFile);
    pMenuBar->addMenu(pEdit);
    pMenuBar->addMenu(pRender);
    pMenuBar->addMenu(pView);
    pMenuBar->addMenu(pWindow);
    pMenuBar->addMenu(pHelp);
    pMenuBar->setProperty("cssClass", "mainWin");

    setMenuBar(pMenuBar);
}

void ZenoMainWindow::initDocks() {
    QWidget *p = takeCentralWidget();
    if (p)
        delete p;

    setDockNestingEnabled(true);

    m_viewDock = new ZenoDockWidget("view", this);
    m_viewDock->setObjectName(QString::fromUtf8("dock_view"));
    m_viewDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    DisplayWidget *view = new DisplayWidget(this);
    m_viewDock->setWidget(DOCK_VIEW, view);

    m_parameter = new ZenoDockWidget("parameter", this);
    m_parameter->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_parameter->setWidget(DOCK_NODE_PARAMS, new ZenoPropPanel);

    m_editor = new ZenoDockWidget("", this);
    m_editor->setObjectName(QString::fromUtf8("dock_editor"));
    m_editor->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_pEditor = new ZenoGraphsEditor(this);
    m_editor->setWidget(DOCK_EDITOR, m_pEditor);

    m_logger = new ZenoDockWidget("logger", this);
    m_logger->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_logger->setWidget(DOCK_LOG, new ZlogPanel);

    auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (ZenoDockWidget *pDock : docks)
    {
        connect(pDock, SIGNAL(maximizeTriggered()), this, SLOT(onMaximumTriggered()));
        connect(pDock, SIGNAL(splitRequest(bool)), this, SLOT(onSplitDock(bool)));
    }
}

void ZenoMainWindow::onMaximumTriggered()
{
    ZenoDockWidget *pDockWidget = qobject_cast<ZenoDockWidget *>(sender());

    auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (ZenoDockWidget *pDock : docks)
    {
        if (pDock != pDockWidget)
        {
            pDock->close();
        }
    }
}

void ZenoMainWindow::onSplitDock(bool bHorzontal)
{
    ZenoDockWidget *pDockWidget = qobject_cast<ZenoDockWidget *>(sender());
    ZenoDockWidget *pDock = new ZenoDockWidget("", this);
    pDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);

    if (ZenoGraphsEditor *pEditor = qobject_cast<ZenoGraphsEditor *>(pDockWidget->widget()))
    {
        ZenoGraphsEditor *pEditor2 = new ZenoGraphsEditor(this);
        pDock->setWidget(DOCK_EDITOR, pEditor2);
        //only one model.
        pEditor2->resetModel(zenoApp->graphsManagment()->currentModel());
        splitDockWidget(pDockWidget, pDock, bHorzontal ? Qt::Horizontal : Qt::Vertical);
    }
    else
    {
        splitDockWidget(pDockWidget, pDock, bHorzontal ? Qt::Horizontal : Qt::Vertical);
    }
    connect(pDock, SIGNAL(maximizeTriggered()), this, SLOT(onMaximumTriggered()));
    connect(pDock, SIGNAL(splitRequest(bool)), this, SLOT(onSplitDock(bool)));
}

void ZenoMainWindow::openFileDialog() {
    QString filePath = getOpenFileByDialog();
    if (filePath.isEmpty())
        return;

    //todo: path validation
    saveQuit();
    openFile(filePath);
}

void ZenoMainWindow::onNewFile() {
    saveQuit();
    zenoApp->graphsManagment()->newFile();
}

void ZenoMainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    adjustDockSize();
}

void ZenoMainWindow::adjustDockSize() {
    //temp: different layout
    float height = size().height();
    int dockHeightA = 0.50 * height;
    int dockHeightB = 0.50 * height;

    QList<QDockWidget *> docks = {m_viewDock, m_editor};
    QList<int> dockSizes = {dockHeightA, dockHeightB};
    resizeDocks(docks, dockSizes, Qt::Vertical);
}

void ZenoMainWindow::importGraph() {
    QString filePath = getOpenFileByDialog();
    if (filePath.isEmpty())
        return;

    //todo: path validation
    auto pGraphs = zenoApp->graphsManagment();
    pGraphs->importGraph(filePath);
}

static bool saveContent(const QString &strContent, QString filePath) {
    QFile f(filePath);
    zeno::log_debug("saving {} chars to file [{}]", strContent.size(), filePath.toStdString());
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << Q_FUNC_INFO << "Failed to open" << filePath << f.errorString();
        zeno::log_error("Failed to open file for write: {} ({})", filePath.toStdString(),
                        f.errorString().toStdString());
        return false;
    }
    f.write(strContent.toUtf8());
    f.close();
    zeno::log_debug("saved successfully");
    return true;
}

void ZenoMainWindow::exportGraph() {
    QString path = QFileDialog::getSaveFileName(this, "Path to Save", "",
                                                "C++ Source File(*.cpp);; JSON file(*.json);; All Files(*);;");
    if (path.isEmpty()) {
        return;
    }

    //auto pGraphs = zenoApp->graphsManagment();
    //pGraphs->importGraph(path);

    QString content;
    {
        rapidjson::StringBuffer s;
        RAPIDJSON_WRITER writer(s);
        IGraphsModel *pModel = zenoApp->graphsManagment()->currentModel();
        GraphsModel *model = (GraphsModel *)pModel;
        {
            JsonArrayBatch batch(writer);
            serializeScene(model, writer);
        }
        if (path.endsWith(".cpp")) {
            content = translateGraphToCpp(s.GetString(), s.GetLength(), model);
        } else {
            content = QString(s.GetString());
        }
    }
    saveContent(content, path);
}

bool ZenoMainWindow::openFile(QString filePath) {
    auto pGraphs = zenoApp->graphsManagment();
    IGraphsModel *pModel = pGraphs->openZsgFile(filePath);
    if (!pModel)
        return false;
    return true;
}

void ZenoMainWindow::onToggleDockWidget(DOCK_TYPE type, bool bShow)
{
    auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (ZenoDockWidget *dock : docks)
    {
        DOCK_TYPE _type = dock->type();
        if (_type == type)
            dock->setVisible(bShow);
    }
}

void ZenoMainWindow::onDockSwitched(DOCK_TYPE type)
{
    ZenoDockWidget *pDock = qobject_cast<ZenoDockWidget *>(sender());
    switch (type)
    {
        case DOCK_EDITOR: {
            ZenoGraphsEditor *pEditor2 = new ZenoGraphsEditor(this);
            pEditor2->resetModel(zenoApp->graphsManagment()->currentModel());
            pDock->setWidget(type, pEditor2);
            break;
        }
        case DOCK_VIEW: {
            //complicated opengl framework.
            //DisplayWidget* view = new DisplayWidget;
            //pDock->setWidget(type, view);
            break;
        }
        case DOCK_NODE_PARAMS: {
            ZenoPropPanel *pWidget = new ZenoPropPanel;
            pDock->setWidget(type, pWidget);
            break;
        }
        case DOCK_NODE_DATA: {
            QWidget *pWidget = new QWidget;
            QPalette pal = pWidget->palette();
            pal.setColor(QPalette::Window, QColor(0, 0, 255));
            pWidget->setAutoFillBackground(true);
            pWidget->setPalette(pal);
            pDock->setWidget(type, pWidget);
            break;
        }
        case DOCK_LOG: {
            ZlogPanel* pPanel = new ZlogPanel;
            pDock->setWidget(type, pPanel);
            break;
        }
    }
}

void ZenoMainWindow::saveQuit() {
    auto pGraphsMgm = zenoApp->graphsManagment();
    Q_ASSERT(pGraphsMgm);
    IGraphsModel *pModel = pGraphsMgm->currentModel();
    if (pModel && pModel->isDirty()) {
        QMessageBox msgBox =
            QMessageBox(QMessageBox::Question, "Save", "Save changes?", QMessageBox::Yes | QMessageBox::No, this);
        QPalette pal = msgBox.palette();
        pal.setBrush(QPalette::WindowText, QColor(0, 0, 0));
        msgBox.setPalette(pal);
        int ret = msgBox.exec();
        if (ret & QMessageBox::Yes) {
            saveAs();
        }
    }
    pGraphsMgm->clear();
}

void ZenoMainWindow::save() {
    auto pGraphsMgm = zenoApp->graphsManagment();
    Q_ASSERT(pGraphsMgm);
    IGraphsModel *pModel = pGraphsMgm->currentModel();
    if (pModel) {
        QString currFilePath = pModel->filePath();
        if (currFilePath.isEmpty())
            return saveAs();
        saveFile(currFilePath);
    }
}

bool ZenoMainWindow::saveFile(QString filePath) {
    IGraphsModel *pModel = zenoApp->graphsManagment()->currentModel();
    QString strContent = ZsgWriter::getInstance().dumpProgramStr(pModel);
    saveContent(strContent, filePath);
    pModel->setFilePath(filePath);
    pModel->clearDirty();
    return true;
}

bool ZenoMainWindow::inDlgEventLoop() const {
    return m_bInDlgEventloop;
}

void ZenoMainWindow::saveAs() {
    VarToggleScope scope(&m_bInDlgEventloop);
    QString path = QFileDialog::getSaveFileName(this, "Path to Save", "", "Zensim Graph File(*.zsg);; All Files(*);;");
    if (!path.isEmpty()) {
        saveFile(path);
    }
}

QString ZenoMainWindow::getOpenFileByDialog() {
    VarToggleScope scope(&m_bInDlgEventloop);
    const QString &initialPath = ".";
    QFileDialog fileDialog(this, tr("Open"), initialPath, "Zensim Graph File (*.zsg)\nAll Files (*)");
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setDirectory(initialPath);
    if (fileDialog.exec() != QDialog::Accepted)
        return "";

    QString filePath = fileDialog.selectedFiles().first();
    return filePath;
}

void ZenoMainWindow::verticalLayout()
{
    addDockWidget(Qt::TopDockWidgetArea, m_viewDock);
    splitDockWidget(m_viewDock, m_editor, Qt::Vertical);
    splitDockWidget(m_editor, m_parameter, Qt::Horizontal);
    splitDockWidget(m_viewDock, m_logger, Qt::Horizontal);
}

void ZenoMainWindow::onlyEditorLayout()
{
    verticalLayout();
    auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (ZenoDockWidget *dock : docks)
    {
        if (dock->type() != DOCK_EDITOR)
        {
            dock->close();
        }
    }
}

void ZenoMainWindow::writeHoudiniStyleLayout() {
    QSettings settings("Zeno Inc.", "zeno2 ui1");
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
}

void ZenoMainWindow::writeSettings2() {
    QSettings settings("Zeno Inc.", "zeno2 ui2");
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
}

void ZenoMainWindow::readHoudiniStyleLayout() {
    QSettings settings("Zeno Inc.", "zeno2 ui1");
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}

void ZenoMainWindow::readSettings2() {
    QSettings settings("Zeno Inc.", "zeno2 ui2");
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}

void ZenoMainWindow::onRunClicked(int beginFrame, int endFrame) {
    auto pGraphsMgr = zenoApp->graphsManagment();
    IGraphsModel *pModel = pGraphsMgr->currentModel();
    GraphsModel *pLegacy = qobject_cast<GraphsModel *>(pModel);
    launchProgram(pLegacy, beginFrame, endFrame);
}

void ZenoMainWindow::onNodesSelected(const QModelIndex &subgIdx, const QModelIndexList &nodes, bool select) {
    //dispatch to all property panel.
    auto docks = findChildren<ZenoDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (ZenoDockWidget *dock : docks) {
        dock->onNodesSelected(subgIdx, nodes, select);
    }
}
