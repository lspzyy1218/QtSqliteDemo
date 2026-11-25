#ifndef UI_LOGINWIDGET_H
#define UI_LOGINWIDGET_H

#include <QWidget>

class QLineEdit;
class QLabel;

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget* parent = nullptr);

signals:
    void loginSucceeded();

private slots:
    void onLogin();

private:
    void setupUi();
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLabel* m_hint = nullptr;
};

#endif // UI_LOGINWIDGET_H
