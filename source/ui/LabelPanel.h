#pragma once

#include <QWidget>

class QListWidget;
class QLineEdit;
class QPushButton;
class AnnotationData;

class LabelPanel : public QWidget {
    Q_OBJECT
public:
    explicit LabelPanel(AnnotationData* data, QWidget* parent = nullptr);

    int selectedLabelId() const;

private slots:
    void onAddLabel();
    void onRemoveLabel();
    void refreshList();

private:
    AnnotationData* m_data;
    QListWidget*    m_labelList;
    QLineEdit*      m_nameEdit;
    QLineEdit*      m_descEdit;
    QPushButton*    m_addBtn;
    QPushButton*    m_removeBtn;
};
