#include "SettingsManager.h"
SettingsManager::SettingsManager(QObject *parent) : QObject(parent), m_settings(new QSettings("config.ini", QSettings::IniFormat, this)) {}
QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const { return m_settings->value(key, defaultValue); }
void SettingsManager::setValue(const QString &key, const QVariant &value){ m_settings->setValue(key, value); }
void SettingsManager::sync(){ m_settings->sync(); }
bool SettingsManager::contains(const QString &key) const { return m_settings->contains(key); }
void SettingsManager::remove(const QString &key){ m_settings->remove(key); }
