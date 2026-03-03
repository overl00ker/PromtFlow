#include "MainWindow.h"
#include "NodeScene.h"
#include "SettingsDialog.h"
#include "HistoryPanel.h"

#include <QGraphicsView>
#include <QMenuBar>
#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QDir>

class GraphView : public QGraphicsView
{
public:
    explicit GraphView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers() & Qt::ControlModifier) {
            const double zoomFactor = 1.15;
            if (event->angleDelta().y() > 0)
                scale(zoomFactor, zoomFactor);
            else
                scale(1.0 / zoomFactor, 1.0 / zoomFactor);
        } else {
            QGraphicsView::wheelEvent(event);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::MiddleButton) {
            setDragMode(QGraphicsView::ScrollHandDrag);
            QMouseEvent fakeEvent(event->type(), event->localPos(), event->windowPos(), event->screenPos(),
                                  Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
            QGraphicsView::mousePressEvent(&fakeEvent);
        } else {
            QGraphicsView::mousePressEvent(event);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::MiddleButton) {
            QMouseEvent fakeEvent(event->type(), event->localPos(), event->windowPos(), event->screenPos(),
                                  Qt::LeftButton, event->buttons() & ~Qt::LeftButton, event->modifiers());
            QGraphicsView::mouseReleaseEvent(&fakeEvent);
            setDragMode(QGraphicsView::RubberBandDrag);
        } else {
            QGraphicsView::mouseReleaseEvent(event);
        }
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new NodeScene(this);

    m_view = new GraphView(m_scene, this);
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setBackgroundBrush(QColor(30, 30, 30));
    setCentralWidget(m_view);

    createHistoryDock();
    createMenus();


    connect(m_scene, &NodeScene::historyEntryReady,
            m_historyPanel, &HistoryPanel::addEntry);

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    QString autoSavePath = dataDir + "/autosave.json";
    if (QFile::exists(autoSavePath)) {
        m_scene->loadWorkflow(autoSavePath);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    QString autoSavePath = dataDir + "/autosave.json";
    m_scene->saveWorkflow(autoSavePath);
    
    QMainWindow::closeEvent(event);
}

void MainWindow::createHistoryDock()
{
    m_historyPanel = new HistoryPanel(this);

    m_historyDock = new QDockWidget(tr("History"), this);
    m_historyDock->setWidget(m_historyPanel);
    m_historyDock->setMinimumWidth(200);
    m_historyDock->setStyleSheet(
        "QDockWidget { color: #ccc; font-weight: bold; }"
        "QDockWidget::title { background: #2b2b2b; padding: 6px;"
        "  border-bottom: 1px solid #444; }");
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);
    m_historyDock->hide();
}

void MainWindow::createMenus()
{

    auto *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    auto *fullscreenAction = new QAction(tr("Fullscreen Mode"), this);
    fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    fullscreenAction->setCheckable(true);
    connect(fullscreenAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) showFullScreen();
        else showNormal();
    });
    settingsMenu->addAction(fullscreenAction);

    settingsMenu->addSeparator();
    auto *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    settingsMenu->addAction(quitAction);


    auto *apiConfigAction = new QAction(tr("&API"), this);
    apiConfigAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    connect(apiConfigAction, &QAction::triggered, this, [this]() {
        SettingsDialog dlg(this);
        dlg.exec();
    });
    menuBar()->addAction(apiConfigAction);


    auto *workflowMenu = menuBar()->addMenu(tr("&Workflow"));

    auto *loadWorkflowObj = new QAction(tr("Load Workflow..."), this);
    loadWorkflowObj->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(loadWorkflowObj, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Load Workflow", "", "JSON Files (*.json)");
        if (!path.isEmpty()) m_scene->loadWorkflow(path);
    });
    workflowMenu->addAction(loadWorkflowObj);

    auto *saveWorkflowObj = new QAction(tr("Save Workflow..."), this);
    saveWorkflowObj->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(saveWorkflowObj, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Save Workflow", "workflow.json", "JSON Files (*.json)");
        if (!path.isEmpty()) m_scene->saveWorkflow(path);
    });
    workflowMenu->addAction(saveWorkflowObj);


    QAction *toggleHistory = m_historyDock->toggleViewAction();
    toggleHistory->setText(tr("&History"));
    toggleHistory->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    menuBar()->addAction(toggleHistory);

    menuBar()->setStyleSheet(
        "QMenuBar { background: #2b2b2b; color: #ccc; border-bottom: 1px solid #444; }"
        "QMenuBar::item:selected { background: #3c3c3c; }"
        "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; }"
        "QMenu::item:selected { background: #3c78b5; }"
    );
}