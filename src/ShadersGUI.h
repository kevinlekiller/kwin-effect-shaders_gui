/**
 * Copyright (C) 2022  kevinlekiller
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHADERSGUI_H
#define SHADERSGUI_H

#include <QFileSystemWatcher>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSettings>
#include <QTableWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class ShadersGUI; }
QT_END_NAMESPACE

class ShadersGUI : public QMainWindow
{
    Q_OBJECT

public:
    ShadersGUI(QWidget *parent = nullptr);
    ~ShadersGUI();

private:
    void processShaderPath(QString);
    void updateShadersText(QString);
    void updateShadersText(QByteArray);
    void parseShadersText();
    void watchSettingsFile();
    void unWatchSettingsFile();
    void sortProfiles();
    void setProfileActive(QString);
    void createProfileFile(QString);
    void setProfilesToUI();
    void connectToSocket();

    QString m_profilesPath;
    QString m_shaderPath;
    QString m_shaderSettingsPath;
    QString m_oldProfileName;
    const QString m_shaderSettingsName = "1_settings.glsl";
    QByteArray m_prevShadersText;
    QByteArray m_shadersText;
    QFileSystemWatcher m_shaderSettingsWatcher;
    QSettings *m_settings;
    Ui::ShadersGUI *ui;

//private Q_SLOTS:
    void slotCloseWindow();
    void slotShaderSettingsChanged();
    void slotShaderSave();
    void slotMoveShaderUp();
    void slotMoveShaderDown();
    void slotUpdateShaderOrder();
    void slotSettingsSave();
    void slotWhiteListSave();
    void slotProfileCreate();
    void slotProfileDelete();
    void slotProfileCopy();
    void slotProfileChange(int);
    void slotProfileRenamed(QListWidgetItem *);
    void slotProfileMakeEditable(QListWidgetItem *);
    void slotToggleShader(int, int);
    void slotEditShaderSetting(QTableWidgetItem *);
};
#endif // SHADERSGUI_H
