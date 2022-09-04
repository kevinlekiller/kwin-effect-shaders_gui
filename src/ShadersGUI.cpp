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

#include "ShadersGUI.h"
#include "./ui_ShadersGUI.h"
//#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTemporaryFile>

/**
 * @brief Construct.
 */
ShadersGUI::ShadersGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ShadersGUI) {
    ui->setupUi(this);
    // Initialize settings.
    m_settings = new QSettings("kevinlekiller", "kwin_effect_shaders");

    // Set values on UI.
    ui->value_AutoSave->setChecked(m_settings->value("AutoSave").toBool());
    ui->button_ShadersSave->setHidden(m_settings->value("AutoSave").toBool());
    ui->button_OrderSave->setHidden(m_settings->value("AutoSave").toBool());
    ui->value_AutoEnable->setChecked(m_settings->value("AutoEnable").toBool());
    restoreGeometry(m_settings->value("WindowGeometry").toByteArray());
    ui->tabWidget->setCurrentIndex(m_settings->value("LastTab").toInt());
    processShaderPath(m_settings->value("ShaderPath").toString());

    // Setup connections.
    connect(ui->button_CloseWindow, &QDialogButtonBox::clicked, this, &ShadersGUI::slotCloseWindow);
    connect(ui->button_OrderSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotShaderSave);
    connect(ui->button_ShadersSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotShaderSave);
    connect(ui->button_SettingsSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotSettingsSave);
    connect(ui->button_WhiteListSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotWhiteListSave);
    connect(ui->button_MoveShaderUp, &QPushButton::clicked, this, &ShadersGUI::slotMoveShaderUp);
    connect(ui->button_MoveShaderDown, &QPushButton::clicked, this, &ShadersGUI::slotMoveShaderDown);
    connect(ui->button_ProfilesNew, &QPushButton::clicked, this, &ShadersGUI::slotProfileCreate);
    connect(ui->button_ProfilesCopy, &QPushButton::clicked, this, &ShadersGUI::slotProfileCopy);
    connect(ui->button_ProfilesRemove, &QPushButton::clicked, this, &ShadersGUI::slotProfileDelete);
    connect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
    connect(ui->table_Profiles, &QListWidget::itemClicked, this, &ShadersGUI::slotProfileMakeEditable);
    connect(ui->value_ShaderOrder->model(), &QAbstractItemModel::rowsMoved, this, &ShadersGUI::slotUpdateShaderOrder);
    connect(ui->value_profileDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ShadersGUI::slotProfileChange);
    connect(ui->table_Shaders, &QTableWidget::cellClicked, this, &ShadersGUI::slotToggleShader);
    connect(ui->table_Shaders, &QTableWidget::itemChanged, this, &ShadersGUI::slotEditShaderSetting);
}

/**
 * @brief Destruct.
 */
ShadersGUI::~ShadersGUI() {
    m_settings->setValue("WindowGeometry", saveGeometry());
    m_settings->setValue("LastTab", ui->tabWidget->currentIndex());
    delete m_settings;
    delete ui;
}

/**
 * @brief If we changed the shader settings file, tell kwin_effect_shaders.
 */
void ShadersGUI::connectToServer() {
    QLocalSocket socket;
    socket.setServerName("kwin_effect_shaders");
    socket.connectToServer();
    socket.waitForConnected(250);
    socket.close();
}

/**
 * @brief Close the window.
 */
void ShadersGUI::slotCloseWindow() {
    close();
}

/**
 * @brief Process user specified shader path.
 * The path should contain glsl files ending with glsl, frag and vert extensions.
 * The path must contain the 1_settings.glsl.example file.
 *
 * @param shaderPath -> The shader path to process.
 */
void ShadersGUI::processShaderPath(QString shaderPath) {
    shaderPath = shaderPath.trimmed();
    if (shaderPath.isEmpty()) {
        shaderPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kwin-effect-shaders_shaders", QStandardPaths::LocateDirectory);
        if (shaderPath.isEmpty()) {
            return;
        }
    }
    if (!shaderPath.endsWith("/")) {
        shaderPath.append("/");
    }
    if (QString::compare(m_shaderPath, shaderPath) == 0) {
        return;
    }
    m_shaderPath = shaderPath;
    QDir shadersDir(m_shaderPath);
    if (!shadersDir.isReadable()) {
        return;
    }
    m_settings->setValue("ShaderPath", m_shaderPath);
    ui->value_ShaderPath->setPlainText(m_shaderPath);

    m_shaderSettingsPath = m_shaderPath;
    m_shaderSettingsPath.append(m_shaderSettingsName);

    // Create Profiles dir.
    shadersDir.mkdir("p");
    m_profilesPath = shadersDir.absolutePath();
    m_profilesPath.append("/p/");
    setProfilesToUI();
}

/**
 * @brief Adds the profiles to the UI.
 */
void ShadersGUI::setProfilesToUI() {
    QDir profilesDir(m_profilesPath);

    // Create iterator of .p files in profiles dir.
    profilesDir.setNameFilters(QStringList() << "*.p");
    profilesDir.setSorting(QDir::Name);
    QStringList profiles(profilesDir.entryList());

    // Reset UI values.
    ui->value_profileDropdown->clear();
    ui->table_Profiles->clear();

    // No profile exists, copy 1_settings.glsl.example to X.p
    if (profiles.isEmpty()) {
        slotProfileCreate();
        // Set profile as current profile.
        setProfileActive(ui->value_profileDropdown->itemText(0));
        ui->value_profileDropdown->setCurrentIndex(0);
        return;
    }
    bool foundActiveProfile = false;
    QString activeProfile = m_settings->value("ActiveProfile").toString();
    // Iterate .p files.
    for (auto &curProfile : profiles) {
        curProfile.chop(2);
        // Set profiles to UI.
        ui->value_profileDropdown->addItem(curProfile);
        ui->table_Profiles->addItem(curProfile);
        if (QString::compare(activeProfile, curProfile) == 0) {
            foundActiveProfile = true;
        }
    }
    sortProfiles();
    if (!foundActiveProfile) {
        setProfileActive(ui->value_profileDropdown->itemText(0));
        ui->value_profileDropdown->setCurrentIndex(0);
    } else {
        setProfileActive(activeProfile);
    }
}

/**
 * @brief User clicked button to create a new profile.
 */
void ShadersGUI::slotProfileCreate() {
    QString examplePath(m_shaderSettingsPath);
    examplePath.append(".example");
    createProfileFile(examplePath);
}

/**
 * @brief User clicked button to copy selected profile.
 */
void ShadersGUI::slotProfileCopy() {
    // No item selected.
    if (ui->table_Profiles->currentRow() < 0) {
        return;
    }
    QListWidgetItem *currentItem(ui->table_Profiles->currentItem());
    QString profileName(currentItem->text().trimmed());
    if (profileName.isEmpty()) {
        return;
    }
    QString profilePath(m_profilesPath);
    profilePath.append(profileName).append(".p");
    createProfileFile(profilePath);
}

/**
 * @brief Creates a new profile based on a existing profile or the example profile.
 * @param originalFilePath : Path to the original file that will be copied to make the new profile.
 */
void ShadersGUI::createProfileFile(QString originalFilePath) {
    // Create file with unique name.
    QTemporaryFile newProfile;
    newProfile.setFileTemplate("ProfileXXXXXX.p");
    newProfile.setAutoRemove(false);
    // This creates the file.
    newProfile.open();
    newProfile.close();
    // Delete the file, or it will fail when we try to copy.
    QFile::remove(newProfile.fileName());
    QFile oldFile(originalFilePath);
    // Copy example file to new profile.
    if (!oldFile.copy(newProfile.fileName())) {
        return;
    }
    // Get the filename without path.
    QFileInfo fInfo(newProfile.fileName());
    QString newProfileName(fInfo.fileName());
    QString newProfilePath(m_profilesPath);
    newProfilePath.append(newProfileName);
    // Move the file to the profiles dir.
    QFile::rename(newProfile.fileName(), newProfilePath);
    // Remove .p extension.
    newProfileName.chop(2);
    // Add profile to UI.
    ui->value_profileDropdown->addItem(newProfileName);
    ui->table_Profiles->addItem(newProfileName);
    sortProfiles();
}

/**
 * @brief User clicked button to delete selected profile.
 */
void ShadersGUI::slotProfileDelete() {
    // No item selected.
    if (ui->table_Profiles->currentRow() < 0) {
        return;
    }
    // Don't remove last profile.
    if (ui->table_Profiles->count() <= 1) {
        return;
    }
    QListWidgetItem *currentItem(ui->table_Profiles->currentItem());
    QString profileName(currentItem->text().trimmed());
    if (profileName.isEmpty()) {
        return;
    }
    // Remove profile from UI.
    ui->table_Profiles->removeItemWidget(currentItem);
    int comboIndex = ui->value_profileDropdown->findText(profileName);
    if (comboIndex > -1) {
        ui->value_profileDropdown->removeItem(comboIndex);
    }
    // Delete the actual file.
    QString profilePath(m_profilesPath);
    profilePath.append(profileName).append(".p");
    QFile::remove(profilePath);
    setProfilesToUI();
}

/**
 * @brief Links the profile file to make it active.
 * @param profile : Name of the profile to set active.
 */
void ShadersGUI::setProfileActive(QString profile) {
    QString profilePath(m_profilesPath);
    profilePath.append(profile).append(".p");
    QFile profileFile(profilePath);
    if (!profileFile.exists()) {
        return;
    }
    unWatchSettingsFile();
    QFile::remove(m_shaderSettingsPath);
    QFile::link(profilePath, m_shaderSettingsPath);
    QFile settingsFile(m_shaderSettingsPath);
    if (!settingsFile.exists() || !settingsFile.open(QFile::ReadOnly)) {
        return;
    }
    m_settings->setValue("ActiveProfile", profile);
    m_settings->sync();
    m_shadersText = settingsFile.readAll();
    watchSettingsFile();
    ui->value_profileDropdown->setCurrentIndex(ui->value_profileDropdown->findText(profile));
    parseShadersText();
}

/**
 * @brief User changed the profile using the dropdown QComboBox.
 * @param int index
 */
void ShadersGUI::slotProfileChange(int index) {
    QString profileName(ui->value_profileDropdown->itemText(index));
    if (profileName.isEmpty()) {
        return;
    }
    setProfileActive(profileName);
}

/**
 * @brief If any changes are made to the profiles, re-sort them.
 */
void ShadersGUI::sortProfiles() {
    ui->table_Profiles->sortItems();
    ui->value_profileDropdown->model()->sort(0, Qt::AscendingOrder);
}

/**
 * @brief User renamed the profile.
 * @param item
 */
void ShadersGUI::slotProfileRenamed(QListWidgetItem *item) {
    QString oldProfileName = m_oldProfileName;
    m_oldProfileName = item->text();
    if (item->text().isEmpty() || QString::compare(item->text(), oldProfileName) == 0 || ui->value_profileDropdown->findText(item->text()) >= 0) {
        disconnect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
        item->setText(oldProfileName);
        connect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
        return;
    }
    QString newProfilePath(m_shaderPath);
    newProfilePath.append("p/").append(item->text()).append(".p");
    QFile newProfileFile(newProfilePath);
    if (newProfileFile.exists()) {
        disconnect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
        item->setText(oldProfileName);
        connect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
        return;
    }
    QString oldProfilePath(m_shaderPath);
    oldProfilePath.append("p/").append(oldProfileName).append(".p");
    QFile oldProfileFile(oldProfilePath);
    if (!oldProfileFile.copy(newProfilePath)) {
        return;
    }
    QFile::remove(oldProfilePath);
    if (QString::compare(m_settings->value("ActiveProfile").toString(), oldProfileName) == 0) {
        setProfileActive(item->text());
        ui->value_profileDropdown->setItemText(ui->value_profileDropdown->findText(oldProfileName), item->text());
        sortProfiles();
        ui->value_profileDropdown->setCurrentIndex(ui->value_profileDropdown->findText(item->text()));
        return;
    }
    ui->value_profileDropdown->setItemText(ui->value_profileDropdown->findText(oldProfileName), item->text());
    sortProfiles();
}

/**
 * @brief QListWidget is non editable, so set the item to be editable any time the user clicks it.
 *        Stores the current profile name in case the user is going to edit that profile name.
 * @param item
 */
void ShadersGUI::slotProfileMakeEditable(QListWidgetItem *item) {
    disconnect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_oldProfileName = item->text();
    connect(ui->table_Profiles, &QListWidget::itemChanged, this, &ShadersGUI::slotProfileRenamed);
}

/**
 * @brief Stop watching for changes on the shaders settings file.
 */
void ShadersGUI::unWatchSettingsFile() {
    if (m_shaderSettingsPath.isEmpty()) {
        return;
    }
    if (m_shaderSettingsWatcher.files().contains(m_shaderSettingsPath)) {
        m_shaderSettingsWatcher.removePath(m_shaderSettingsPath);
        disconnect(&m_shaderSettingsWatcher, &QFileSystemWatcher::fileChanged, this, &ShadersGUI::slotShaderSettingsChanged);
    }
}

/**
 * @brief Watch for changes on the shaders settings file.
 */
void ShadersGUI::watchSettingsFile() {
    if (m_shaderSettingsPath.isEmpty()) {
        return;
    }
    if (m_shaderSettingsWatcher.addPath(m_shaderSettingsPath)) {
        connect(&m_shaderSettingsWatcher, &QFileSystemWatcher::fileChanged, this, &ShadersGUI::slotShaderSettingsChanged);
    }
}

/**
 * @brief Update the shader setting buffer.
 * @param shadersText
 */
void ShadersGUI::updateShadersText(QString shadersText) {
    QByteArray text(shadersText.toUtf8());
    updateShadersText(text);
}

/**
 * @brief Update the shader setting buffer.
 * @param shadersText
 */
void ShadersGUI::updateShadersText(QByteArray shadersText) {
    if (shadersText.operator==(m_shadersText)) {
        return;
    }
    m_shadersText.swap(shadersText);
    if (m_settings->value("AutoSave").toBool()) {
        slotShaderSave();
    }
}

/**
 * @brief User requested saving the shader settings.
 */
void ShadersGUI::slotShaderSave() {
    unWatchSettingsFile();
    QFile settingsFile(m_shaderSettingsPath);
    if (!settingsFile.exists() || !settingsFile.open(QFile::WriteOnly | QFile::Truncate)) {
        return;
    }
    settingsFile.write(m_shadersText);
    watchSettingsFile();
    connectToServer();
}

/**
 * @brief User requested moving shader up in order list.
 */
void ShadersGUI::slotMoveShaderUp() {
    int row = ui->value_ShaderOrder->currentRow();
    QListWidgetItem *item = ui->value_ShaderOrder->takeItem(row);
    ui->value_ShaderOrder->insertItem(--row, item);
    slotUpdateShaderOrder();
    ui->value_ShaderOrder->setCurrentRow(row);
}

/**
 * @brief User requested moving shader down in order list.
 */
void ShadersGUI::slotMoveShaderDown() {
    int row = ui->value_ShaderOrder->currentRow();
    QListWidgetItem *item = ui->value_ShaderOrder->takeItem(row);
    ui->value_ShaderOrder->insertItem(++row, item);
    slotUpdateShaderOrder();
    ui->value_ShaderOrder->setCurrentRow(row);
}

/**
 * @brief User requested saving main settings.
 */
void ShadersGUI::slotSettingsSave() {
    m_settings->sync();
    m_settings->setValue("ShaderPath", ui->value_ShaderPath->toPlainText());
    m_settings->setValue("AutoSave", ui->value_AutoSave->isChecked());
    ui->button_ShadersSave->setHidden(ui->value_AutoSave->isChecked());
    ui->button_OrderSave->setHidden(ui->value_AutoSave->isChecked());
    m_settings->setValue("AutoEnable", ui->value_AutoEnable->isChecked());
    m_settings->sync();
}

/**
 * @brief User requested saving the Whitelist.
 */
void ShadersGUI::slotWhiteListSave() {
    m_settings->sync();
    QString whiteList(ui->value_Whitelist->toPlainText().trimmed().replace(QString("\n"), QString(" ")).replace(QString("\t"), QString(" ")));
    m_settings->setValue("Whitelist", whiteList);
    m_settings->sync();
    QString whiteListReplacement("//WHITELIST=\"");
    whiteListReplacement.append(whiteList).append("\"");
    QRegularExpression replaceRegex("^//WHITELIST=\".*?\"", QRegularExpression::MultilineOption);
    QString shadersText(m_shadersText);
    shadersText.replace(replaceRegex, whiteListReplacement);
    updateShadersText(shadersText);
}

/**
 * @brief User requested to enable or disable a shader.
 *
 * @param row    -> The row in the shader table.
 * @param column -> The column in the shader table.
 */
void ShadersGUI::slotToggleShader(int row, int column) {
    QString settingName = ui->table_Shaders->item(row, 0)->text().trimmed();
    if (!settingName.startsWith("SHADER_")) {
        return;
    }

    QString value = ui->table_Shaders->item(row, 1)->text();
    int on = QString::compare(value, "On");
    if (on != 0 && QString::compare(value, "Off") != 0) {
        return;
    }

    QString shadersText = m_shadersText;
    QString regex = "^#define\\s+";
    settingName.remove(0, 7);
    regex.append(settingName).append("_ENABLED\\s+\\d+");
    QRegularExpression replaceRegex(regex, QRegularExpression::MultilineOption);
    settingName.prepend("#define ").append("_ENABLED ").append(on == 0 ? "0" : "1");
    shadersText.replace(replaceRegex, settingName);
    updateShadersText(shadersText);
    parseShadersText();
}

/**
 * @brief User wants to edit a shader setting.
 *
 * @param item -> The item in the shader table.
 */
void ShadersGUI::slotEditShaderSetting(QTableWidgetItem *item) {
    if (item->column() == 0) {
        return;
    }

    QString settingName = ui->table_Shaders->item(item->row(), 0)->text().trimmed();
    if (settingName.startsWith("SHADER_")) {
        return;
    }

    QString settingValue = item->text().trimmed();
    if (settingValue.isEmpty()) {
        return;
    }

    QString replacement, regex, verificationRegex;
    if (settingValue.startsWith("vec3(")) {
        verificationRegex = "^vec3\\((-?\\d+\\.\\d+\\,\\s*){2}-?\\d+\\.\\d+\\)$";
        replacement.append("uniform vec3 ").append(settingName).append(" = ").append(settingValue).append(";");
        regex.append("^uniform\\s+vec3\\s+").append(settingName).append("\\s+=\\s+.+?").append(";");
    } else if (settingValue.startsWith("vec2(")) {
        verificationRegex = "^vec2\\(-?\\d+\\.\\d+\\,\\s*-?\\d+\\.\\d+\\)$";
        replacement.append("uniform vec2 ").append(settingName).append(" = ").append(settingValue).append(";");
        regex.append("^uniform\\s+vec2\\s+").append(settingName).append("\\s+=\\s+.+?").append(";");
    } else {
        verificationRegex = "^(-?\\d+|-?\\d+\\.\\d+)$";
        replacement.append("#define ").append(settingName).append(" ").append(settingValue);
        regex.append("^#define\\s+").append(settingName).append("\\s+[\\d.-]+");
    }
    // Validate input.
    QRegularExpression rev(verificationRegex);
    QRegularExpressionMatch matches = rev.match(settingValue);
    if (!matches.hasMatch()) {
        parseShadersText();
        return;
    }
    QString shadersText = QString(m_shadersText);
    QRegularExpression replaceRegex(regex, QRegularExpression::MultilineOption);
    shadersText.replace(replaceRegex, replacement);
    updateShadersText(shadersText);
    parseShadersText();
}

/**
 * @brief If the user moves shaders up or down, update that here.
 */
void ShadersGUI::slotUpdateShaderOrder() {
    if (!ui->value_ShaderOrder->count()) {
        return;
    }
    QString order = "const int SHADER_ORDER[SHADERS+1] = int[] ( // Don't change this line.\n\n";
    for (int i = 0; i < ui->value_ShaderOrder->count(); ++i) {
        order.append("    SHADER_").append(ui->value_ShaderOrder->item(i)->text()).append(",\n");
    }
    order.append("\nSHADERS); //");
    QRegularExpression orderRegex("^const\\s+int\\s+SHADER_ORDER.+?^SHADERS\\);\\s+//", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);
    QString shadersText = QString(m_shadersText);
    shadersText = shadersText.replace(orderRegex, order);
    updateShadersText(shadersText);
    connectToServer();
}

/**
 * @brief Process the shader settings, set variables to the UI.
 */
void ShadersGUI::parseShadersText() {
    QStringList lines = QString(m_shadersText).split("\n");
    if (lines.empty()) {
        return;
    }
    int curTableRow = 0;
    ui->table_Shaders->clearContents();
    ui->table_Shaders->setRowCount(curTableRow);

    QRegularExpression enabledRegex("^#define\\s+([A-Z0-9_]+)_ENABLED\\s+(\\d)");
    QRegularExpression setting1Regex("^#define\\s+([A-Z0-9_]+)\\s+(\\S+)");
    QRegularExpression setting2Regex("^uniform\\s+.+?\\s+([A-Z0-9_]+)\\s+=\\s+(.+?);\\s*$");

    disconnect(ui->table_Shaders, &QTableWidget::itemChanged, this, &ShadersGUI::slotEditShaderSetting);
    bool foundOrder = false, foundDefine = false, foundDesc = false, foundSource = false, foundWhitelist = false, curShaderEnabled = false;
    QStringList shaderOrder;
    QString curShader, enabledShaders, curTooltip, shaderTooltip;
    for (int i = 0; i < lines.size(); ++i) {
        QString curLine = lines.at(i).trimmed();

        // Set the shader order on the UI.
        if (!foundOrder) {
            if (curLine.startsWith("SHADERS);")) {
                foundOrder = true;
                continue;
            }
            if (curLine.startsWith("SHADER_")) {
                curLine.remove(0, 7);
                if (curLine.endsWith(",")) {
                    curLine.chop(1);
                }
                shaderOrder.append(curLine);
                continue;
            }
        }

        // Find the whitelist.
        if (!foundWhitelist && curLine.startsWith("//WHITELIST=\"")) {
            foundWhitelist = true;
            QString whiteList = curLine;
            whiteList.remove(0, 13).chop(1);
            ui->value_Whitelist->setPlainText(whiteList);
            slotWhiteListSave();
            continue;
        }

        // Get tooltip for the shader.
        if (!foundSource) {
            if (curLine.startsWith("// Source: ")) {
                foundSource = true;
            } else if (!foundDesc && curLine.startsWith("// Description: ")) {
                foundDesc = true;
            }
            if (!foundDesc) {
                continue;
            }
            // Remove the leading //
            curLine = curLine.remove(0, 2).trimmed();
            if (curLine.startsWith("Description: ")) {
                shaderTooltip.append("<p>").append(curLine);
            } else if (curLine.startsWith("Source: ")) {
                shaderTooltip.append("</p><p>").append(curLine).append("</p>");
            } else if (curLine.startsWith("License: ")) {
                shaderTooltip.append("<p>").append(curLine).append("</p>");
            } else {
                shaderTooltip.append(" ").append(curLine);
            }
            continue;
        }

        // Find the start of shader's settings.
        if (!foundDefine) {
            QRegularExpressionMatch matches = enabledRegex.match(curLine);
            if (matches.hasMatch()) {
               curShader = matches.captured(1).prepend("SHADER_");
               curShaderEnabled = matches.captured(2).toInt() == 1;
               ui->table_Shaders->insertRow(curTableRow);
               QTableWidgetItem *curShaderItem = new QTableWidgetItem(curShader);
               curShaderItem->setFlags(curShaderItem->flags() & ~Qt::ItemIsEditable);
               ui->table_Shaders->setItem(curTableRow, 0, curShaderItem);
               QTableWidgetItem *settingItem = new QTableWidgetItem(curShaderEnabled ? "On" : "Off");
               settingItem->setFlags(settingItem->flags() & ~Qt::ItemIsEditable);
               ui->table_Shaders->setItem(curTableRow, 1, settingItem);
               if (!shaderTooltip.isEmpty()) {
                   shaderTooltip.prepend("<html><head/><body>").append("</body></html>");
                   settingItem->setToolTip(shaderTooltip);
               }
               curTableRow++;
               if (curShaderEnabled) {
                   QString shortShader;
                   shortShader.append(curShader).remove(0, 7);
                   enabledShaders.append(shortShader).append(", ");
               }
               foundDefine = true;
            }
            continue;
        }

        // Reached the end of this shader's settings.
        if (curLine.startsWith("#endif")) {
            foundDefine = false;
            foundSource = false;
            foundDesc = false;
            shaderTooltip.clear();
            continue;
        }

        // Skip adding the setting / tooltip to the UI if the shader is disabled.
        if (!curShaderEnabled) {
            continue;
        }

        // Get the tooltip for the current setting.
        if (curLine.startsWith("//")) {
            curLine = curLine.remove(0, 2).trimmed();
            curTooltip.append("<p>").append(curLine).append("</p>");
        }

        // Put the setting onto the UI with its tooltip.
        bool isDefine = curLine.startsWith("#define");
        if (isDefine || curLine.startsWith("uniform")) {
            QRegularExpressionMatch matches;
            matches.operator=(isDefine? setting1Regex.match(curLine) : setting2Regex.match(curLine));
            if (matches.hasMatch()) {
                ui->table_Shaders->insertRow(curTableRow);
                QTableWidgetItem *nameItem = new QTableWidgetItem(matches.captured(1));
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                QTableWidgetItem *settingItem = new QTableWidgetItem(matches.captured(2));
                curTooltip = curTooltip.trimmed();
                if (!curTooltip.isEmpty()) {
                    curTooltip.prepend("<html><head/><body>").append("</body></html>");
                    nameItem->setToolTip(curTooltip);
                    settingItem->setToolTip(curTooltip);
                }
                ui->table_Shaders->setItem(curTableRow, 0, nameItem);
                ui->table_Shaders->setItem(curTableRow, 1, settingItem);
                curTableRow++;
            }
            curTooltip.clear();
        }
    }
    connect(ui->table_Shaders, &QTableWidget::itemChanged, this, &ShadersGUI::slotEditShaderSetting);
    // Set enabled shaders list on the status tab.
    if (enabledShaders.endsWith(", ")) {
        enabledShaders.chop(2);
        ui->value_ShadersEnabled->setText(enabledShaders);
    } else {
        ui->value_ShadersEnabled->setText("");
    }
    // Set the data on the shader order tab.
    ui->value_ShaderOrder->clear();
    ui->value_ShaderOrder->addItems(shaderOrder);
    // Tell kwin_effect_shaders to reload.
    connectToServer();
}

/**
 * @brief Reparse shader setting file if it's modified.
 */
void ShadersGUI::slotShaderSettingsChanged() {
    parseShadersText();
}
