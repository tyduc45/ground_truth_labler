#include "LabelPanel.h"
#include "core/AnnotationData.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QColorDialog>

static const QColor s_colors[] = {
    Qt::green, Qt::red, Qt::blue, Qt::yellow, Qt::cyan,
    Qt::magenta, QColor(255, 128, 0), QColor(128, 0, 255)
};
static const int s_numColors = sizeof(s_colors) / sizeof(s_colors[0]);

LabelPanel::LabelPanel(AnnotationData* data, QWidget* parent)
    : QWidget(parent), m_data(data)
{
    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel(tr("Labels"));
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel(tr("Name:")));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(tr("e.g. person"));
    layout->addWidget(m_nameEdit);

    layout->addWidget(new QLabel(tr("Description:")));
    m_descEdit = new QLineEdit;
    m_descEdit->setPlaceholderText(tr("e.g. pedestrian"));
    layout->addWidget(m_descEdit);

    auto* btnLayout = new QHBoxLayout;
    m_addBtn    = new QPushButton(tr("Add"));
    m_removeBtn = new QPushButton(tr("Remove"));
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    layout->addLayout(btnLayout);

    // Color picker row
    auto* colorRow = new QHBoxLayout;
    colorRow->addWidget(new QLabel(tr("Color:")));
    m_colorBtn = new QPushButton;
    m_colorBtn->setFixedWidth(40);
    m_colorBtn->setFixedHeight(22);
    m_colorBtn->setToolTip(tr("Click to change label color"));
    colorRow->addWidget(m_colorBtn);
    colorRow->addStretch();
    layout->addLayout(colorRow);

    m_labelList = new QListWidget;
    layout->addWidget(m_labelList, 1);

    setFixedWidth(200);

    connect(m_addBtn,    &QPushButton::clicked, this, &LabelPanel::onAddLabel);
    connect(m_removeBtn, &QPushButton::clicked, this, &LabelPanel::onRemoveLabel);
    connect(m_colorBtn,  &QPushButton::clicked, this, &LabelPanel::onChangeColor);
    connect(m_labelList, &QListWidget::currentRowChanged,
            this, [this](int) { onLabelSelectionChanged(); });
    connect(m_data, &AnnotationData::labelsChanged, this, &LabelPanel::refreshList);
}

int LabelPanel::selectedLabelId() const
{
    auto* item = m_labelList->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void LabelPanel::onAddLabel()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Label name cannot be empty."));
        return;
    }

    LabelDef label;
    label.id          = m_data->nextLabelId();
    label.name        = name;
    label.description = m_descEdit->text().trimmed();
    label.color       = s_colors[(label.id - 1) % s_numColors];

    m_data->addLabel(label);
    m_nameEdit->clear();
    m_descEdit->clear();
}

void LabelPanel::onRemoveLabel()
{
    int labelId = selectedLabelId();
    if (labelId < 0) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a label to remove."));
        return;
    }
    m_data->removeLabel(labelId);
}

void LabelPanel::onChangeColor()
{
    int id = selectedLabelId();
    if (id < 0) {
        QMessageBox::information(this, tr("Info"), tr("Please select a label first."));
        return;
    }
    QColor current;
    for (const auto& lbl : m_data->labels()) {
        if (lbl.id == id) { current = lbl.color; break; }
    }
    QColor chosen = QColorDialog::getColor(current, this, tr("Select Label Color"));
    if (chosen.isValid())
        m_data->updateLabelColor(id, chosen);
}

void LabelPanel::onLabelSelectionChanged()
{
    int id = selectedLabelId();
    if (id < 0) {
        m_colorBtn->setStyleSheet("background-color: #888;");
        return;
    }
    for (const auto& lbl : m_data->labels()) {
        if (lbl.id == id) {
            m_colorBtn->setStyleSheet(
                QString("background-color: %1; border: 1px solid #555;")
                    .arg(lbl.color.name()));
            return;
        }
    }
}

void LabelPanel::refreshList()
{
    m_labelList->clear();
    for (const auto& label : m_data->labels()) {
        auto* item = new QListWidgetItem(
            QString("[%1] %2").arg(label.id).arg(label.name));
        item->setData(Qt::UserRole, label.id);
        item->setForeground(label.color);
        m_labelList->addItem(item);
    }
    if (m_labelList->count() > 0)
        m_labelList->setCurrentRow(0);
    onLabelSelectionChanged();
}
