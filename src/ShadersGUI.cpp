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
#include <QDir>
#include <QFile>
#include <QScrollBar>
#include <QStandardPaths>

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
    ui->value_Blacklist->setPlainText(m_settings->value("Blacklist").toString());
    ui->value_Whitelist->setPlainText(m_settings->value("Whitelist").toString());
    ui->value_AutoSave->setChecked(m_settings->value("AutoSave").toBool());
    ui->button_ShadersSave->setHidden(m_settings->value("AutoSave").toBool());
    ui->button_OrderSave->setHidden(m_settings->value("AutoSave").toBool());
    ui->value_AutoEnable->setChecked(m_settings->value("AutoEnable").toBool());
    restoreGeometry(m_settings->value("WindowGeometry").toByteArray());
    ui->tabWidget->setCurrentIndex(m_settings->value("LastTab").toInt());
    processShaderPath(m_settings->value("ShaderPath").toString());
    parseShadersText();

    // Setup connections.
    connect(ui->button_CloseWindow, &QDialogButtonBox::clicked, this, &ShadersGUI::slotCloseWindow);
    connect(ui->button_OrderSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotShaderSave);
    connect(ui->button_ShadersSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotShaderSave);
    connect(ui->button_SettingsSave, &QDialogButtonBox::clicked, this, &ShadersGUI::slotSettingsSave);
    connect(ui->button_MoveShaderUp, &QPushButton::clicked, this, &ShadersGUI::slotMoveShaderUp);
    connect(ui->button_MoveShaderDown, &QPushButton::clicked, this, &ShadersGUI::slotMoveShaderDown);
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
    QDir shadersDir(shaderPath);
    if (!shadersDir.isReadable()) {
        return;
    }
    ui->value_ShaderPath->setPlainText(m_shaderPath);
    watchSettingsFile();
    m_shaderSettingsPath = m_shaderPath;
    m_shaderSettingsPath.append(m_shaderSettingsName);
    if (!shadersDir.exists(m_shaderSettingsName)) {
        QString exampleName = m_shaderSettingsPath;
        exampleName.append(".example");
        QFile exampleFile(exampleName);
        if (!exampleFile.exists() || !exampleFile.copy(m_shaderSettingsPath)) {
            exampleFile.close();
            return;
        }
        exampleFile.close();
    }
    QFile settingsFile(m_shaderSettingsPath);
    if (!settingsFile.exists() || !settingsFile.open(QFile::ReadOnly)) {
        return;
    }
    m_shadersText = settingsFile.readAll();
    m_settings->setValue("ShaderPath", m_shaderPath);
    unWatchSettingsFile();
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
    QByteArray text = shadersText.toUtf8();
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
}

/**
 * @brief User requested moving shader up in order list.
 */
void ShadersGUI::slotMoveShaderUp() {
    int row = ui->value_ShaderOrder->currentRow();
    QListWidgetItem *item = ui->value_ShaderOrder->takeItem(row);
    ui->value_ShaderOrder->insertItem(--row, item);
    updateShaderOrder();
    ui->value_ShaderOrder->setCurrentRow(row);
}

/**
 * @brief User requested moving shader down in order list.
 */
void ShadersGUI::slotMoveShaderDown() {
    int row = ui->value_ShaderOrder->currentRow();
    QListWidgetItem *item = ui->value_ShaderOrder->takeItem(row);
    ui->value_ShaderOrder->insertItem(++row, item);
    updateShaderOrder();
    ui->value_ShaderOrder->setCurrentRow(row);
}

/**
 * @brief User requested saving main settings.
 */
void ShadersGUI::slotSettingsSave() {
    m_settings->sync();
    m_settings->setValue("ShaderPath", ui->value_ShaderPath->toPlainText());
    m_settings->setValue("Blacklist", ui->value_Blacklist->toPlainText());
    m_settings->setValue("Whitelist", ui->value_Whitelist->toPlainText());
    m_settings->setValue("AutoSave", ui->value_AutoSave->isChecked());
    ui->button_ShadersSave->setHidden(ui->value_AutoSave->isChecked());
    ui->button_OrderSave->setHidden(ui->value_AutoSave->isChecked());
    m_settings->setValue("DefaultEnabled", ui->value_AutoEnable->isChecked());
    m_settings->sync();
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

    QString replacement, regex;
    if (settingValue.startsWith("vec3(")) {
        replacement.append("uniform vec3 ").append(settingName).append(" = ").append(settingValue).append(";");
        regex.append("^uniform\\s+vec3\\s+").append(settingName).append("\\s+=\\s+.+?").append(";");
    } else {
        replacement.append("#define ").append(settingName).append(" ").append(settingValue);
        regex.append("^#define\\s+").append(settingName).append("\\s+[\\d.]+");
    }
    QRegularExpression replaceRegex(regex, QRegularExpression::MultilineOption);
    QString shadersText = QString(m_shadersText);
    shadersText.replace(replaceRegex, replacement);
    updateShadersText(shadersText);
    parseShadersText();
}

/**
 * @brief If the user moves shaders up or down, update that here.
 */
void ShadersGUI::updateShaderOrder() {
    if (!ui->value_ShaderOrder->count()) {
        return;
    }
    QString shadersText = QString(m_shadersText);
    QString order = "const int SHADER_ORDER[SHADERS+1] = int[] ( // Don't change this line.\n\n";
    for (int i = 0; i < ui->value_ShaderOrder->count(); ++i) {
           order.append("    SHADER_").append(ui->value_ShaderOrder->item(i)->text()).append(",\n");
    }
    order.append("\nSHADERS); //");
    QRegularExpression orderRegex("^const\\s+int\\s+SHADER_ORDER.+?^SHADERS\\);\\s+//", QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);
    shadersText = shadersText.replace(orderRegex, order);
    updateShadersText(shadersText);
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
    bool foundOrder = false, foundDefine = false, curShaderEnabled = false;
    QStringList shaderOrder;
    QString curShader, enabledShaders, curTooltip;
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
    }
    // Set the data on the shader order tab.
    ui->value_ShaderOrder->clear();
    ui->value_ShaderOrder->addItems(shaderOrder);
}

/**
 * @brief Reparse shader setting file if it's modified.
 */
void ShadersGUI::slotShaderSettingsChanged() {
    parseShadersText();
}
