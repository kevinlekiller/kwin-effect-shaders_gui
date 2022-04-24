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
#include <QSystemSemaphore>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QSystemSemaphore sem("kwin-effect-shader_gui-sem", 1, QSystemSemaphore::Open);
    if (!sem.acquire()) {
        return EXIT_FAILURE;
    }
    QSharedMemory shm("kwin-effect-shader_gui-shm");
    if (shm.isAttached()) {
        shm.detach();
    }
    if (shm.attach()) {
        sem.release();
        return EXIT_SUCCESS;
    } else {
        shm.create(1);
    }
    sem.release();
    ShadersGUI w;
    w.show();
    int ret = a.exec();
    shm.detach();
    return ret;
}
