#ifndef UI_SETTINGSPAGE_H
#define UI_SETTINGSPAGE_H

#include "ipage.h"

class QLineEdit;
class QCheckBox;
class QComboBox;

// 参数配置：admin 全部可改；管理员/普通用户仅“基础信息”“检测参数”可改
class SettingsPage : public IPage
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);
    QString pageTitle() const override { return QStringLiteral("参数配置"); }
    Permission requiredPermission() const override { return Permission::Settings; }
    void onEnter() override;

private slots:
    void onSave();

private:
    void setupUi();
    void refresh();
    struct Field {
        QString key;
        QLineEdit* edit = nullptr;
        QComboBox* combo = nullptr;
        QCheckBox* check = nullptr;
        QString category;
    };
    QVector<Field> m_fields;
};

#endif // UI_SETTINGSPAGE_H
