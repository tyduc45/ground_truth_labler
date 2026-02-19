#include "ControlBar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

ControlBar::ControlBar(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    // Frame slider row
    auto* sliderLayout = new QHBoxLayout;
    m_frameLabel = new QLabel("Frame: 0 / 0");
    m_frameSlider = new QSlider(Qt::Horizontal);
    m_frameSlider->setEnabled(false);
    sliderLayout->addWidget(m_frameSlider, 1);
    sliderLayout->addWidget(m_frameLabel);
    mainLayout->addLayout(sliderLayout);

    // Buttons row
    auto* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    m_runBtn = new QPushButton(tr("Run"));
    m_stopBtn = new QPushButton(tr("Stop"));
    m_acceptBtn = new QPushButton(tr("Accept"));
    m_undoBtn = new QPushButton(tr("Undo"));

    m_runBtn->setMinimumWidth(80);
    m_stopBtn->setMinimumWidth(80);
    m_acceptBtn->setMinimumWidth(80);
    m_undoBtn->setMinimumWidth(80);

    m_runBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    m_stopBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_acceptBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    m_undoBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");

    btnLayout->addWidget(m_runBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addWidget(m_acceptBtn);
    btnLayout->addWidget(m_undoBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // Initial state: all disabled
    m_runBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
    m_acceptBtn->setEnabled(false);
    m_undoBtn->setEnabled(false);

    connect(m_runBtn, &QPushButton::clicked, this, &ControlBar::runClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &ControlBar::stopClicked);
    connect(m_acceptBtn, &QPushButton::clicked, this, &ControlBar::acceptClicked);
    connect(m_undoBtn, &QPushButton::clicked, this, &ControlBar::undoClicked);
    connect(m_frameSlider, &QSlider::valueChanged, this, [this](int val) {
        m_frameLabel->setText(QString("Frame: %1 / %2").arg(val).arg(m_frameSlider->maximum()));
        emit frameSliderChanged(val);
    });
}

void ControlBar::setRunEnabled(bool en) { m_runBtn->setEnabled(en); }
void ControlBar::setStopEnabled(bool en) { m_stopBtn->setEnabled(en); }
void ControlBar::setAcceptEnabled(bool en) { m_acceptBtn->setEnabled(en); }
void ControlBar::setUndoEnabled(bool en) { m_undoBtn->setEnabled(en); }

void ControlBar::setFrameRange(int min, int max)
{
    m_frameSlider->setRange(min, max);
    m_frameLabel->setText(QString("Frame: %1 / %2").arg(m_frameSlider->value()).arg(max));
}

void ControlBar::setCurrentFrame(int frame)
{
    m_frameSlider->blockSignals(true);
    m_frameSlider->setValue(frame);
    m_frameSlider->blockSignals(false);
    m_frameLabel->setText(QString("Frame: %1 / %2").arg(frame).arg(m_frameSlider->maximum()));
}

void ControlBar::setSliderEnabled(bool en)
{
    m_frameSlider->setEnabled(en);
}
