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
#include <QApplication>
#include <QSharedMemory>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QSharedMemory shm("kwin-effect-shader_gui-shm");
    if (shm.attach(QSharedMemory::ReadOnly)) {
        // In case previous process died unexpectedly. https://doc.qt.io/qt-5/qsharedmemory.html#details
        shm.detach();
        if (shm.attach(QSharedMemory::ReadOnly)) {
            return EXIT_SUCCESS;
        }
    }
    if (!shm.create(1, QSharedMemory::ReadOnly)) {
        return EXIT_FAILURE;
    }
    ShadersGUI w;
    w.show();
    int ret = a.exec();
    shm.detach();
    return ret;
}
