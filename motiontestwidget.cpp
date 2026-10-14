#include "motiontestwidget.h"

#include <QColor>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QtGlobal>

MotionTestWidget::MotionTestWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("IMU - rysowanie (C = wyczysc)"));
    resize(680, 480);
    setMinimumSize(320, 240);
    setFocusPolicy(Qt::StrongFocus);
}

void MotionTestWidget::resetPoint()
{
    m_point = rect().center();
    m_hasPoint = true;
    m_strokeOpen = false;  // Nigdy nie laczymy linii ze starym polozeniem.
    update();
}

void MotionTestWidget::setDrawingEnabled(bool enabled)
{
    if (m_drawingEnabled == enabled)
        return;

    m_drawingEnabled = enabled;
    m_strokeOpen = false; // OFF konczy kreske; nastepne ON zacznie nowa.
    update();
}

void MotionTestWidget::clearDrawing()
{
    m_strokes.clear();
    m_strokeOpen = false;
    update();
}

void MotionTestWidget::applyDelta(const QPointF &delta)
{
    if (!m_hasPoint)
        resetPoint();

    if (delta.isNull())
        return;

    constexpr double margin = 10.0;
    const QPointF oldPoint = m_point;
    m_point += delta;
    m_point.setX(qBound(margin, m_point.x(), width() - margin));
    m_point.setY(qBound(margin + 24.0, m_point.y(), height() - margin));

    if (m_drawingEnabled) {
        if (!m_strokeOpen) {
            QPolygonF stroke;
            stroke << oldPoint;  // Punkt przed ruchem, kreska bez skoku.
            m_strokes.append(stroke);
            m_strokeOpen = true;
        }
        m_strokes.last() << m_point;
    }
    update();
}

void MotionTestWidget::setStatusText(const QString &text)
{
    if (m_status == text)
        return;
    m_status = text;
    update();
}

void MotionTestWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_hasPoint) {
        constexpr double margin = 10.0;
        m_point.setX(qBound(margin, m_point.x(), width() - margin));
        m_point.setY(qBound(margin + 24.0, m_point.y(), height() - margin));
    }
    m_strokeOpen = false;
}

void MotionTestWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_C) {
        clearDrawing();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MotionTestWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(250, 250, 252));

    painter.setPen(m_drawingEnabled ? QColor(20, 140, 65) : QColor(95, 95, 100));
    painter.drawText(QRect(12, 8, width() - 24, 24),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     m_status + (m_drawingEnabled ? " | RYSOWANIE ON" : " | RYSOWANIE OFF"));

    painter.setPen(QColor(220, 220, 225));
    painter.drawRect(rect().adjusted(1, 33, -2, -2));

    QPen strokePen(QColor(30, 30, 35), 2.5);
    strokePen.setCapStyle(Qt::RoundCap);
    strokePen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(strokePen);
    for (const QPolygonF &stroke : m_strokes) {
        if (stroke.size() > 1)
            painter.drawPolyline(stroke);
        else if (stroke.size() == 1)
            painter.drawPoint(stroke.first());
    }

    const QPointF point = m_hasPoint ? m_point : QPointF(rect().center());
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_drawingEnabled ? QColor(20, 140, 65) : QColor(35, 108, 225));
    painter.drawEllipse(point, 7.0, 7.0);
}
