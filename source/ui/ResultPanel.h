#pragma once

#include <QWidget>

class QListWidget;
class QPushButton;
class AnnotationData;

class ResultPanel : public QWidget {
    Q_OBJECT
public:
    explicit ResultPanel(AnnotationData* data, QWidget* parent = nullptr);

signals:
    void segmentDoubleClicked(int index);
    void mergeRequested();
    void exportMotRequested();

private slots:
    void refreshList();
    void onItemDoubleClicked();

private:
    AnnotationData* m_data;
    QListWidget*    m_segmentList;
    QPushButton*    m_mergeBtn;
    QPushButton*    m_exportBtn;
};
