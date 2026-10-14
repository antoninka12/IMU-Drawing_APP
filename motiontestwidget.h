#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QString>
#include <QVector>
#include <QWidget>

// Okno rysowania: punkt rusza sie zawsze, kreski tylko w trybie DRAWING ON.
class MotionTestWidget : public QWidget
{
public:
    explicit MotionTestWidget(QWidget *parent = nullptr);

    void applyDelta(const QPointF &delta);
    void resetPoint();
    void setStatusText(const QString &text);
    void setDrawingEnabled(bool enabled);
    void clearDrawing();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPointF m_point;
    bool m_hasPoint = false;
    bool m_drawingEnabled = false;
    bool m_strokeOpen = false;
    QVector<QPolygonF> m_strokes;
    QString m_status = QStringLiteral("Polacz BLE i trzymaj czujnik nieruchomo");
};
