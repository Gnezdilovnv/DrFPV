#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H
#include <QObject>
#include <QSettings>
#include <QVariant>
class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent=nullptr);
    QVariant value(const QString &key, const QVariant &defaultValue=QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    void sync();
    bool contains(const QString &key) const;
    void remove(const QString &key);
private:
    QSettings *m_settings;
};
#endif
