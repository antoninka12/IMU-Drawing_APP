#include "motiontestwidget.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QtGlobal>

MotionTestWidget::MotionTestWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("IMU - test ruchu punktu"));
    resize(520, 380);
    setMinimumSize(320, 240);
}

void MotionTestWidget::resetPoint()
{
    m_point = rect().center();
    m_hasPoint = true;
    update();
}

void MotionTestWidget::applyDelta(const QPointF &delta)
{
    if (!m_hasPoint)
        resetPoint();

    if (delta.isNull())
        return;

    constexpr double margin = 10.0;
    m_point += delta;
    m_point.setX(qBound(margin, m_point.x(), width() - margin));
    m_point.setY(qBound(margin + 24.0, m_point.y(), height() - margin));
    update();
}

void MotionTestWidget::setStatusText(const QString &text)
{
    if (m_status != text) {
        m_status = text;
        update();
    }
}

void MotionTestWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_hasPoint) {
        constexpr double margin = 10.0;
        m_point.setX(qBound(margin, m_point.x(), width() - margin));
        m_point.setY(qBound(margin + 24.0, m_point.y(), height() - margin));
    }
}

void MotionTestWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(250, 250, 252));

    painter.setPen(QColor(95, 95, 100));
    painter.drawText(QRect(12, 8, width() - 24, 24),
                     Qt::AlignLeft | Qt::AlignVCenter, m_status);

    painter.setPen(QColor(220, 220, 225));
    painter.drawRect(rect().adjusted(1, 33, -2, -2));

    const QPointF point = m_hasPoint ? m_point : QPointF(rect().center());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(35, 108, 225));
    painter.drawEllipse(point, 8.0, 8.0);
}
