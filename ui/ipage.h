#ifndef UI_IPAGE_H
#define UI_IPAGE_H

#include <QWidget>
#include "core/models.h"

// 所有功能页面实现的接口
class IPage : public QWidget
{
    Q_OBJECT
public:
    explicit IPage(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~IPage() = default;

    virtual QString pageTitle() const = 0;
    virtual Permission requiredPermission() const = 0;
    virtual void onEnter() {} // 每次进入页面时刷新数据
};

#endif // UI_IPAGE_H
