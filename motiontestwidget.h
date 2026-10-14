#pragma once

#include <QPointF>
#include <QString>
#include <QWidget>

// Okno testowe: wyswietla JEDEN punkt, bez tworzenia linii.
class MotionTestWidget : public QWidget
{
public:
    explicit MotionTestWidget(QWidget *parent = nullptr);

    void applyDelta(const QPointF &delta);
    void resetPoint();
    void setStatusText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPointF m_point;
    bool m_hasPoint = false;
    QString m_status = QStringLiteral("Polacz BLE i trzymaj czujnik nieruchomo");
};
