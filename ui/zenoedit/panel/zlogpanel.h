#ifndef __ZLOGPANEL_H__
#define __ZLOGPANEL_H__

namespace Ui {
    class LogPanel;
}

#include <QtWidgets>

class LogItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    typedef QStyledItemDelegate _base;
public:
    LogItemDelegate(QObject *parent = nullptr);
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;

private:
    QAbstractItemView *m_view;
};

class LogListView : public QListView
{
    Q_OBJECT
    typedef QListView _base;
public:
    LogListView(QWidget *parent = nullptr);

public slots:
    void onCustomContextMenu(const QPoint& point);

protected:
    void rowsInserted(const QModelIndex &parent, int start, int end) override;

private:
    QTimer m_timer;
};

class CustomFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit CustomFilterProxyModel(QObject* parnet = nullptr);
    void setFilters(const QVector<QtMsgType>& filters, const QString& content);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QVector<QtMsgType> m_filters;
    QString m_searchContent;
};

class ZlogPanel : public QWidget
{
    Q_OBJECT
public:
    ZlogPanel(QWidget* parent = nullptr);

private slots:
    void onFilterChanged();

private:
    void initSignals();
    void initModel();

    Ui::LogPanel* m_ui;
    CustomFilterProxyModel *m_pFilterModel;
};


#endif