#pragma once

#include <QMainWindow>

class QGraphicsView;
class QDockWidget;
class NodeScene;
class HistoryPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createMenus();
    void createHistoryDock();

    NodeScene      *m_scene        = nullptr;
    QGraphicsView  *m_view         = nullptr;
    QDockWidget    *m_historyDock  = nullptr;
    HistoryPanel   *m_historyPanel = nullptr;
};
