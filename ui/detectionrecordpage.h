#ifndef UI_DETECTIONRECORDPAGE_H
#define UI_DETECTIONRECORDPAGE_H

#include "ipage.h"

class QTableWidget;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QLabel;
class QChartView;

// 检测记录：查询/导出 + 统计图表（批注2）
class DetectionRecordPage : public IPage
{
    Q_OBJECT
public:
    explicit DetectionRecordPage(QWidget* parent = nullptr);

    QString pageTitle() const override { return QStringLiteral("检测记录"); }
    Permission requiredPermission() const override { return Permission::DetectionRecord; }
    void onEnter() override;

private slots:
    void onQuery();
    void onReset();
    void onExport();

private:
    void setupUi();
    void refreshStats();

    QLineEdit* m_plateEdit = nullptr;
    QLineEdit* m_deviceEdit = nullptr;
    QComboBox* m_resultCombo = nullptr;
    QDateEdit* m_from = nullptr;
    QDateEdit* m_to = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_summary = nullptr;
    QWidget* m_todayChart = nullptr;
    QWidget* m_weekChart = nullptr;
};

#endif // UI_DETECTIONRECORDPAGE_H
