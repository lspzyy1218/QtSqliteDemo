#ifndef UI_DETECTIONPAGE_H
#define UI_DETECTIONPAGE_H

#include "ipage.h"

class QLabel;
class QTableWidget;
class QSpinBox;
class QComboBox;

// 检测模式：单车模式 / 多车模式，手持车辆探测仪 + 地面传感器
class DetectionPage : public IPage
{
    Q_OBJECT
public:
    explicit DetectionPage(QWidget* parent = nullptr);

    QString pageTitle() const override { return QStringLiteral("检测模式"); }
    Permission requiredPermission() const override { return Permission::DetectionMode; }
    void onEnter() override;

private slots:
    void onModeChanged(int index);
    void onSimulate();

private:
    void setupUi();
    void rebuildDevicePanel();
    QComboBox* m_modeCombo = nullptr;
    QSpinBox* m_countSpin = nullptr;
    QTableWidget* m_resultTable = nullptr;
    QWidget* m_devicePanel = nullptr;
};

#endif // UI_DETECTIONPAGE_H
