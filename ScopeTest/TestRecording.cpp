/**
 * Copyright (C) 2010  Ellis Whitehead
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "TestRecording.h"

#include <QApplication>
#include <QTimer>
#include <QTimerEvent>

#include <IdacDriver/IdacSettings.h>


TestRecording::TestRecording(int id) : TestBase(id, true)
{
	m_nRecordings = 0;

	connect(scope, SIGNAL(isRecordingChanged(bool)), this, SLOT(on_scope_isRecordingChanged(bool)));
	//connect(m_timerRecord, SIGNAL(timeout()), this, SLOT(record()));

	startTimer(0);
}

void TestRecording::timerEvent(QTimerEvent* e)
{
	killTimer(e->timerId());
	run();
}

void TestRecording::run()
{
	Actions* actions = scope->actions();

	ui->s = "ScopeTest.ead";
	actions->fileSave->trigger();

	Globals->idacSettings()->nRecordingDuration = 30;
	record();
}

void TestRecording::record()
{
	m_nRecordings++;
	if (m_nRecordings < 100) {
		qDebug() << "RECORD #" << m_nRecordings;
		scope->actions()->recordRecord->trigger();
		QTimer::singleShot(10 * 1000, this, SLOT(save()));
	}
	else
		QApplication::exit();
}

void TestRecording::save()
{
	scope->actions()->recordSave->trigger();
	QTimer::singleShot(2000, this, SLOT(record()));
}

void TestRecording::on_scope_isRecordingChanged(bool bRecording)
{
	if (!bRecording) {
		//QTimer::singleShot(1000, this, SLOT(record()));
	}
}
