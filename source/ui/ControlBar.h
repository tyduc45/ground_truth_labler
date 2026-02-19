#pragma once

#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;

class ControlBar : public QWidget {
    Q_OBJECT
public:
    explicit ControlBar(QWidget* parent = nullptr);

    void setRunEnabled(bool en);
    void setStopEnabled(bool en);
    void setAcceptEnabled(bool en);
    void setUndoEnabled(bool en);

    // Frame slider
    void setFrameRange(int min, int max);
    void setCurrentFrame(int frame);
    void setSliderEnabled(bool en);

signals:
    void runClicked();
    void stopClicked();
    void acceptClicked();
    void undoClicked();
    void frameSliderChanged(int frame);

private:
    QPushButton* m_runBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_acceptBtn;
    QPushButton* m_undoBtn;
    QSlider*     m_frameSlider;
    QLabel*      m_frameLabel;
};
