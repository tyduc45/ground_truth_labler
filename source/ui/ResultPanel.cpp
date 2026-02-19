#include "ResultPanel.h"
#include "core/AnnotationData.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

ResultPanel::ResultPanel(AnnotationData* data, QWidget* parent)
    : QWidget(parent), m_data(data)
{
    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel(tr("Results"));
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(titleLabel);

    m_segmentList = new QListWidget;
    m_segmentList->setIconSize(QSize(120, 80));
    m_segmentList->setViewMode(QListWidget::ListMode);
    m_segmentList->setSpacing(4);
    layout->addWidget(m_segmentList, 1);

    m_mergeBtn = new QPushButton(tr("Merge && Export Video"));
    m_exportBtn = new QPushButton(tr("Export MOT CSV"));
    layout->addWidget(m_mergeBtn);
    layout->addWidget(m_exportBtn);

    setFixedWidth(250);

    connect(m_segmentList, &QListWidget::itemDoubleClicked,
            this, &ResultPanel::onItemDoubleClicked);
    connect(m_mergeBtn, &QPushButton::clicked,
            this, &ResultPanel::mergeRequested);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &ResultPanel::exportMotRequested);
    connect(m_data, &AnnotationData::segmentsChanged,
            this, &ResultPanel::refreshList);
}

void ResultPanel::refreshList()
{
    m_segmentList->clear();
    const auto& segments = m_data->segments();
    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];
        auto* item = new QListWidgetItem;
        item->setText(seg.title);
        item->setData(Qt::UserRole, static_cast<int>(i));
        if (!seg.thumbnail.isNull()) {
            item->setIcon(QIcon(QPixmap::fromImage(seg.thumbnail)));
        }
        m_segmentList->addItem(item);
    }
}

void ResultPanel::onItemDoubleClicked()
{
    auto* item = m_segmentList->currentItem();
    if (item) {
        int index = item->data(Qt::UserRole).toInt();
        emit segmentDoubleClicked(index);
    }
}
