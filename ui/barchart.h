#ifndef UI_BARCHART_H
#define UI_BARCHART_H

#include <QWidget>
#include <QVector>
#include <QString>

// 轻量柱状图（QPainter 自绘，不依赖 Qt Charts）
class BarChart : public QWidget
{
    Q_OBJECT
public:
    explicit BarChart(QWidget* parent = nullptr);

    void setData(const QVector<QString>& labels, const QVector<int>& values);
    void setTitle(const QString& title);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QString> m_labels;
    QVector<int> m_values;
    QString m_title;
};

#endif // UI_BARCHART_H
