#include "barchart.h"

#include <QPainter>
#include <QFontMetrics>

BarChart::BarChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(180);
}

void BarChart::setData(const QVector<QString>& labels, const QVector<int>& values)
{
    m_labels = labels;
    m_values = values;
    update();
}

void BarChart::setTitle(const QString& title)
{
    m_title = title;
    update();
}

void BarChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect area = rect().adjusted(10, 26, -10, -28);

    // 标题
    QFont bold = p.font();
    bold.setBold(true);
    p.setFont(bold);
    p.drawText(QRect(rect().left() + 10, 4, width() - 20, 20), Qt::AlignLeft | Qt::AlignVCenter, m_title);
    p.setFont(QFont());

    if (m_values.isEmpty()) {
        p.drawText(area, Qt::AlignCenter, QStringLiteral("暂无数据"));
        return;
    }

    // 坐标轴
    p.setPen(QPen(QColor(180, 180, 180)));
    p.drawLine(area.bottomLeft(), area.bottomRight());
    p.drawLine(area.topLeft(), area.bottomLeft());

    int maxValue = 1;
    for (int v : m_values) maxValue = qMax(maxValue, v);
    const double step = area.height() / static_cast<double>(maxValue);
    const int n = m_values.size();
    const double slot = area.width() / static_cast<double>(n);
    const double barWidth = qMin(slot * 0.6, 42.0);

    for (int i = 0; i < n; ++i) {
        const double x = area.left() + slot * i + (slot - barWidth) / 2;
        const double h = m_values[i] * step;
        const QRectF bar(x, area.bottom() - h, barWidth, h);
        p.fillRect(bar, QColor(26, 111, 181));
        p.setPen(QPen(QColor(20, 80, 140)));
        p.drawRect(bar);

        p.setPen(QPen(QColor(60, 60, 60)));
        const QRectF labelRect(x - slot * 0.2, area.bottom() + 2, slot * 1.4, 22);
        p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, m_labels.value(i));
    }
}
