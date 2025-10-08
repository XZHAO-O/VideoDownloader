#pragma once

#include <QWidget>
#include <QPixmap>

class LogoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogoWidget(QWidget* parent = nullptr);
    ~LogoWidget();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_logoPixmap;
};
