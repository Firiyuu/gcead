/**
 * Copyright (C) 2008, 2009  Ellis Whitehead
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

#include "MainWindow.h"

#include <math.h>

#include <QtDebug>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QQuickView>
#include <QSettings>
#include <QStackedLayout>
#include <QTextStream>

#include <WaveInfo.h>
#include <Idac/IdacFactory.h>
#include <Idac/IdacProxy.h>

#include "Actions.h"
#include "AppDefines.h"
#include "ChartWidget.h"
#include "Check.h"
#include "EadFile.h"
#include "Globals.h"
#include "ImportEadDialog.h"
#include "ImportRecordDialog.h"
#include "MainScope.h"
#include "MainWindowUi.h"
#include "PanelTabs.h"
#include "RecordHandler.h"
#include "TaskFilterWidget.h"
#include "TaskMarkersWidget.h"
#include "TaskPanel.h"
#include "TaskPublishWidget.h"
#include "TaskReviewWidget.h"
#include "ViewTabs.h"


MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	// REFACTOR: This should be moved to main and the MainScope object passed to MainWindow
	IdacProxy* idac = IdacFactory::getProxy();

	m_scope = new MainScope(new MainWindowUi(this), idac, this);

	readSettings();
	setupActions();
	setupWidgets();

	connect(idac, SIGNAL(statusTextChanged(QString)), m_lblIdacStatus, SLOT(setText(QString)));
	connect(idac, SIGNAL(statusErrorChanged(QString)), this, SLOT(idac_statusErrorChanged(QString)));
	connect(m_scope, SIGNAL(fileChanged(EadFile*)), this, SLOT(scope_fileChanged()));
	connect(m_scope, SIGNAL(isRecentFilesMenuEnabledChanged(bool)), m_recentFilesMenu, SLOT(setEnabled(bool)));
	connect(m_scope, SIGNAL(waveListChanged()), this, SLOT(updateReview()));
	connect(m_scope, SIGNAL(taskTypeChanged(EadTask)), this, SLOT(scope_taskTypeChanged(EadTask)));
	connect(m_scope, SIGNAL(viewTypeChanged(EadView)), this, SLOT(updateReview()));
	connect(m_scope, SIGNAL(commentChanged(const QString&)), this, SLOT(scope_commentChanged()));
	connect(m_scope, SIGNAL(windowTitleChanged(const QString&)), this, SLOT(setWindowTitle(const QString&)));
	connect(m_scope, SIGNAL(isWindowModifiedChanged(bool)), this, SLOT(setWindowModified(bool)));

	// Setup initial values from m_scope
	setWindowTitle(m_scope->windowTitle());
	scope_fileChanged();
	m_recentFilesMenu->setEnabled(m_scope->isRecentFilesMenuEnabled());
	scope_taskTypeChanged(m_scope->taskType());
	actions_viewChartRecording_changed();

	idac->setup();
}

MainWindow::~MainWindow()
{
	writeSettings();
}

void MainWindow::setupWidgets()
{
	//
	// Setup task widgets
	//
	m_paneltabs = new PanelTabs(m_scope);
	
	m_chart = new ChartWidget(m_scope);
	m_chart->setStatusBar(statusBar());

	m_taskReview = new TaskReviewWidget(m_scope);
	m_taskReview->setupItems(NULL);

	//m_taskRecord = new TaskRecordWidget(m_idac, m_chart);
	//connect(m_taskRecord, SIGNAL(saveRecording(bool)), this, SIGNAL(saveRecording(bool)));

	m_taskFilter = new TaskFilterWidget(m_scope);

	m_taskMarkers = new TaskMarkersWidget(m_scope);

	m_taskPublish = new TaskPublishWidget(m_scope);
	m_taskPublish->setChartWidget(m_chart);

	m_taskStack = new QStackedLayout();
	m_taskStack->setSpacing(0);
	m_taskStack->addWidget(new TaskPanel(m_taskReview));
	m_taskStack->addWidget(new TaskPanel(m_taskMarkers));
	m_taskStack->addWidget(new TaskPanel(m_taskPublish));
	m_taskStack->addWidget(new TaskPanel(m_taskFilter));
	m_taskStack->setCurrentIndex(1);

	connect(m_taskPublish, SIGNAL(settingsChanged()), m_scope->chart(), SLOT(redraw()));

	//
	// Setup view widgets
	//
	m_viewtabs = new ViewTabs(m_scope);

	QGridLayout* layout = new QGridLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(m_paneltabs, 0, 0);
	layout->addWidget(m_viewtabs, 0, 1);
	layout->addLayout(m_taskStack, 1, 0);
	layout->addWidget(m_chart, 1, 1);
	layout->setColumnStretch(1, 1);

	QWidget* w = new QWidget;
	w->setLayout(layout);
	setCentralWidget(w);

	m_lblIdacStatus = new QLabel();
	statusBar()->addPermanentWidget(m_lblIdacStatus);
}

void MainWindow::open(const QString& sFilename)
{
	m_scope->open(sFilename);
}

void MainWindow::closeEvent(QCloseEvent* e)
{
	bool bIgnore = false;

	if (m_scope->isRecording())
	{
		QMessageBox::StandardButton btn = QMessageBox::question(
			this,
			tr("Discard"),
			tr("Are you sure you want to discard the current recording?"),
			QMessageBox::Discard | QMessageBox::Cancel,
			QMessageBox::Discard);

		if (btn == QMessageBox::Cancel)
			bIgnore = true;
	}
	
	if (!m_scope->checkSaveAndContinue())
		bIgnore = true;

	if (bIgnore)
		e->ignore();
	else
		e->accept();
}

void MainWindow::setupActions()
{
	Actions* actions = m_scope->actions();

	ui.mnuFile->addAction(actions->fileNew);
	ui.mnuFile->addSeparator();
	ui.mnuFile->addAction(actions->fileOpen);
	m_recentFilesMenu = new QMenu(tr("Open &Recent"), this);
	ui.mnuFile->addMenu(m_recentFilesMenu);
	foreach (QAction* act, m_scope->actions()->fileOpenRecentActions)
		m_recentFilesMenu->addAction(act);
	m_recentFilesMenu->setEnabled(false);
	ui.mnuFile->addSeparator();
	ui.mnuFile->addAction(actions->fileSave);
	ui.mnuFile->addAction(actions->fileSaveAs);
	ui.mnuFile->addSeparator();
	ui.mnuFile->addAction(actions->fileComment);
	ui.mnuFile->addSeparator();
	ui.mnuFile->addAction(actions->fileImport);
	ui.mnuFile->addAction(actions->fileExportSignalData);
	ui.mnuFile->addAction(actions->fileExportAmplitudeData);
	ui.mnuFile->addAction(actions->fileExportRetentionData);
	ui.mnuFile->addAction(actions->fileLoadSampleProject);
	ui.mnuFile->addSeparator();
	ui.mnuFile->addAction(actions->fileExit);

	ui.mnuView->addAction(actions->viewViewMode);
	ui.mnuView->addAction(actions->viewMarkersMode);
	ui.mnuView->addAction(actions->viewPublishMode);
	ui.mnuView->addSeparator();
	ui.mnuView->addAction(actions->viewChartAverages);
	ui.mnuView->addAction(actions->viewChartEads);
	ui.mnuView->addAction(actions->viewChartFids);
	ui.mnuView->addAction(actions->viewChartAll);
	ui.mnuView->addAction(actions->viewChartRecording);
	ui.mnuView->addSeparator();
	ui.mnuView->addAction(actions->viewZoomIn);
	ui.mnuView->addAction(actions->viewZoomOut);
	ui.mnuView->addAction(actions->viewZoomFull);
	QMenu* mnu = new QMenu(tr("Scroll"), this);
	mnu->addAction(actions->viewScrollDivLeft);
	mnu->addAction(actions->viewScrollDivRight);
	mnu->addAction(actions->viewScrollPageLeft);
	mnu->addAction(actions->viewScrollPageRight);
	ui.mnuView->addMenu(mnu);
	ui.mnuView->addSeparator();
	ui.mnuView->addAction(actions->viewWaveComments);

	ui.mnuRecord->addAction(actions->recordRecord);
	ui.mnuRecord->addAction(actions->recordHardwareSettings);
	ui.mnuRecord->addAction(actions->recordSave);
	ui.mnuRecord->addAction(actions->recordDiscard);
	ui.mnuRecord->addSeparator();
	ui.mnuRecord->addAction(actions->recordHardwareConnect);

	ui.mnuMarkers->addAction(actions->markersShowFidPeakMarkers);
	ui.mnuMarkers->addAction(actions->markersShowFidPeakArea);
	ui.mnuMarkers->addAction(actions->markersShowFidPeakTime);
	ui.mnuMarkers->addSeparator();
	ui.mnuMarkers->addAction(actions->markersShowEadPeakMarkers);
	ui.mnuMarkers->addAction(actions->markersShowEadPeakAmplitude);
	ui.mnuMarkers->addAction(actions->markersShowEadPeakTimeSpans);
	ui.mnuMarkers->addAction(actions->markersShowEadPeakTimeStamps);
	ui.mnuMarkers->addSeparator();
	ui.mnuMarkers->addAction(actions->markersShowTimeMarkers);

	ui.toolBar->addAction(actions->recordRecord);
	ui.toolBar->addSeparator();
	ui.toolBar->addAction(actions->fileComment);
	m_lblComment = new QLabel;
	ui.toolBar->addWidget(m_lblComment);

	connect(actions->fileImport, SIGNAL(triggered()), this, SLOT(actions_fileImport_triggered()));
	connect(actions->fileExportSignalData, SIGNAL(triggered()), this, SLOT(actions_fileExportSignalData_triggered()));
	connect(actions->fileExportAmplitudeData, SIGNAL(triggered()), this, SLOT(actions_fileExportAmplitudeData_triggered()));
	connect(actions->fileExportRetentionData, SIGNAL(triggered()), this, SLOT(actions_fileExportRetentionData_triggered()));
	connect(actions->fileExit, SIGNAL(triggered()), this, SLOT(actions_fileExit_triggered()));
	connect(actions->viewChartRecording, SIGNAL(changed()), this, SLOT(actions_viewChartRecording_changed()));
}

void MainWindow::readSettings()
{
	QSettings settings("Syntech", APPSETTINGSKEY);
	
	settings.beginGroup("MainWindow");

	bool bMax = settings.value("Maximized", true).toBool();
	if (bMax)
	{
		// NOTE: This is now handled in main.cpp --ellis, 2009-05-04
		//setWindowState(Qt::WindowMaximized);
	}
	// If we're maximized, don't load the size and position values, because the de-maximizing will still have the maximized height
	else
	{
		if (settings.contains("Size"))
			resize(settings.value("Size").toSize());
		if (settings.contains("Position"))
			move(settings.value("Position").toPoint());
		setWindowState(Qt::WindowActive);
	}
	
	settings.endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings("Syntech", APPSETTINGSKEY);
	
	settings.beginGroup("MainWindow");
	settings.setValue("Size", size());
	settings.setValue("Position", pos());
	settings.setValue("Maximized", isMaximized());
	settings.endGroup();

	Globals->writeSettings();
}

void MainWindow::idac_statusErrorChanged(QString sError)
{
	statusBar()->showMessage(sError);
}

void MainWindow::scope_fileChanged()
{
	CHECK_PRECOND_RET(m_scope->file() != NULL);

	updateReview();

	scope_commentChanged();
}

void MainWindow::scope_taskTypeChanged(EadTask task)
{
	m_taskStack->setCurrentIndex((int) task);
	m_paneltabs->setCurrentIndex((int) task);

	// Might need to switch out of Recording view
	// REFACTOR: This should be done in MainScope instead! -- ellis, 2010-03-31
	if (task == EadTask_Publish && m_scope->viewType() == EadView_Recording)
		m_scope->setViewType(EadView_Averages);
	else
		updateReview();
}

void MainWindow::scope_commentChanged()
{
	m_lblComment->setText(m_scope->toolbarComment());
}

void MainWindow::actions_viewChartRecording_changed()
{
	m_viewtabs->setEnabled(EadView_Recording, m_scope->actions()->viewChartRecording->isEnabled());
}

void MainWindow::updateReview()
{
	if (m_scope->taskType() == EadTask_Review)
		m_taskReview->setupItems(m_scope->file());
}

// REFACTOR: This belongs in MainScope
void MainWindow::actions_fileImport_triggered()
{
	CHECK_PRECOND_RET(m_scope->file() != NULL);

	// REFACTOR: This HACK obviously belongs somewhere else
	static QString sLastDir;
	if (sLastDir.isEmpty())
		sLastDir = Globals->lastDir();

	QString sFilename = QFileDialog::getOpenFileName(
		this,
		QObject::tr("Import Wave from Another Project"),
		sLastDir,
		//QObject::tr("GC-EAD and ASC files (*.ead *.asc)"));
		QObject::tr("GC-EAD files (*.ead)"));

	if (sFilename.isEmpty())
		return;

	QFileInfo fi(sFilename);
	if (fi.suffix().toLower() == "ead") {
		importEad(sFilename);
	}
	else if (fi.suffix().toLower() == "asc") {
		importAsc(sFilename);
	}
	else {
		QMessageBox::warning(this, tr("Unknown file extension"), tr("The file you selected has an unrecognized extension."));
	}
}

LoadSaveResult MainWindow::importEad(const QString &sFilename)
{
	EadFile file2;
	const LoadSaveResult result1 = file2.load(sFilename);
	if (result1 != LoadSaveResult_Ok) {
		QMessageBox::critical(this, tr("Error loading file"), tr("Unable to open the file."));
		return result1;
	}

	//m_scope->file()->importWaves(&file2);

	// Show dialog asking for catagories
	ImportEadDialog dlg(file2, this);
	dlg.exec();

	const QMultiMap<int, WaveType>& map = dlg.recordToWaveTypes();
	for (int i = 0; i < file2.recs().size(); i++) {
		const RecInfo* rec2 = file2.recs()[i];
		RecInfo* recNew = NULL;
		foreach (int nType, map.values(i)) {
			if (recNew == NULL) {
				recNew = new RecInfo(m_scope->file(), m_scope->file()->recs().size());
			}
			const WaveType waveType = (WaveType) nType;
			const WaveInfo* wave2 = rec2->wave(waveType);
			WaveInfo* waveNew = recNew->wave(waveType);
			waveNew->copyFrom(wave2);
		}
		if (recNew != NULL) {
			m_scope->file()->addImportedRecording(recNew);
		}
	}

	return LoadSaveResult_Ok;
}

LoadSaveResult MainWindow::importAsc(const QString &sFilename)
{
	QFile file(sFilename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return LoadSaveResult_CouldNotOpen;

	QTextStream str(&file);

	QString sLine;
	sLine = str.readLine().trimmed();
	if (sLine != ";AutoSpike-32 ASCII File")
		return LoadSaveResult_WrongFormat;

	QStringList asNames;
	const QString sNamePrefix = "; Wave data Signal ";
	const QString sFactorPrefix = "; Rec. Factor ";
	const QString sRatePrefix = "; Sample rate ";
	while (!str.atEnd()) {
		sLine = str.readLine().trimmed();
		if (sLine.startsWith(sNamePrefix)) {
			QString sName = sLine.mid(sNamePrefix.size());
			asNames << sName;
		}
	}

	// Show dialog asking for catagories
	ImportRecordDialog dlg(asNames, this);
	dlg.exec();

	QMap<QString, WaveType> map = dlg.map();
	if (map.size() == 0)
		return LoadSaveResult_Ok;

	RecInfo* rec = new RecInfo(m_scope->file(), m_scope->file()->recs().size());
	str.seek(0);
	bool bReadLine = true;
	while (!str.atEnd()) {
		if (bReadLine)
			sLine = str.readLine().trimmed();
		bReadLine = true;

		if (sLine.startsWith(sNamePrefix)) {
			QString sName = sLine.mid(sNamePrefix.size());
			if (map.contains(sName)) {
				// ; Rec. Factor 116.364313
				// ; Sample rate 20.0
				// ; Format :<time>         <Value>
				QString sLine2 = str.readLine().trimmed();
				QString sLine3 = str.readLine().trimmed();
				QString sLine4 = str.readLine().trimmed();

				CHECK_ASSERT_NORET(sLine2.startsWith(sFactorPrefix) && sLine3.startsWith(sRatePrefix));
				if (!sLine2.startsWith(sFactorPrefix) || !sLine3.startsWith(sRatePrefix))
					break;

				QString sFactor = sLine2.mid(sFactorPrefix.size()).trimmed();
				QString sRate = sLine3.mid(sRatePrefix.size()).trimmed();
				bool bOkFactor, bOkRate;
				double nFactor = sFactor.toDouble(&bOkFactor);
				int nRate = (int) sRate.toDouble(&bOkRate);
				CHECK_ASSERT_NORET(bOkFactor && bOkRate);
				if (!bOkFactor || !bOkRate)
					break;

				WaveType waveType = map[sName];
				WaveInfo* wave = rec->wave(waveType);
				QList<short> raw;
				while (!str.atEnd()) {
					sLine = str.readLine().trimmed();
					if (sLine.startsWith(";")) {
						bReadLine = false;
						if (raw.size() > 0) {
							int nSize = (int) raw.size() * 100 / nRate;
							wave->nRawToVoltageFactorNum = nFactor * 100000;
							wave->nRawToVoltageFactorDen = 100000;
							wave->nRawToVoltageFactor = nFactor;
							wave->raw.resize(nSize);
							if (nRate < 100) {
								// Interpolate raw into wave->raw
								double nIndexRatio = double(nRate) / 100;
								wave->raw[0] = raw[0];
								for (int iOut = 1; iOut < nSize; iOut++) {
									double i = iOut * nIndexRatio;
									int i0 = (int) floor(i);
									int i1 = (int) ceil(i);
									if (i1 >= raw.size())
										i1 = raw.size() - 1;

									short n0 = raw[i0];
									short n;
									if (i0 < i1) {
										short n1 = raw[i1];
										double p = (i - i0) / (i1 - i0);
										n = short(n0 + (n1 - n0) * p);
									}
									else {
										n = n0;
									}

									wave->raw[iOut] = n;
								}
							}
							else {
								// Pick values from raw and place them into wave->raw
								double nIndexRatio = double(nRate) / 100;
								wave->raw[0] = raw[0];
								for (int iOut = 1; iOut < nSize; iOut++) {
									int i = int(iOut * nIndexRatio);
									wave->raw[iOut] = raw[i];
								}
							}
							qDebug() << wave->raw;
						}
						break;
					}
					else if (!sLine.isEmpty()) {
						QStringList asValues = sLine.split("\t");
						CHECK_ASSERT_NORET(asValues.size() == 2);
						if (asValues.size() == 2) {
							//QString sTime = asValues[0];
							QString sValue = asValues[1];

							bool bOkValue;
							short nValue = (short) sValue.toInt(&bOkValue);
							CHECK_ASSERT_NORET(bOkValue);

							if (bOkValue) {
								raw << nValue;
							}
						}
					}
				}
			}
		}
	}

	m_scope->file()->addImportedRecording(rec);

	/*
	; Wave data Signal Mix43-1
	; Rec. Factor 116.364313
	; Sample rate 20.0
	; Format :<time>         <Value>
	*/

	return LoadSaveResult_Ok;
}

void MainWindow::actions_fileExportSignalData_triggered()
{
	CHECK_PRECOND_RET(m_scope->file() != NULL);

	QString sFilename = QFileDialog::getSaveFileName(
		this,
		tr("Export Data"),
		Globals->lastDir(),
		tr("Comma Separated Values (*.csv)"));

	if (sFilename.isEmpty())
		return;

	QFileInfo fi(sFilename);
	if (fi.suffix().isEmpty())
		sFilename += ".csv";

	QMessageBox msg(QMessageBox::Information, tr("Exporting..."), tr("The project data is currently being exported."));
	msg.setStandardButtons(QMessageBox::NoButton);
	msg.show();
	// Let window repaint itself before we do the slow process of exporting
	QApplication::processEvents();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m_scope->file()->exportData(sFilename /*, EadFile::CSV*/);
	QApplication::restoreOverrideCursor();
}

// REFACTOR: almost completely duplicates the above function
void MainWindow::actions_fileExportAmplitudeData_triggered()
{
	CHECK_PRECOND_RET(m_scope->file() != NULL);

	QString sFilename = QFileDialog::getSaveFileName(
		this,
		tr("Export EAD Amplitude Data"),
		Globals->lastDir(),
		tr("Comma Separated Values (*.csv)"));

	if (sFilename.isEmpty())
		return;

	QFileInfo fi(sFilename);
	if (fi.suffix().isEmpty())
		sFilename += ".csv";

	QMessageBox msg(QMessageBox::Information, tr("Exporting..."), tr("The amplitude data is currently being exported."));
	msg.setStandardButtons(QMessageBox::NoButton);
	msg.show();
	// Let window repaint itself before we do the slow process of exporting
	QApplication::processEvents();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m_scope->file()->exportAmplitudeData(sFilename /*, EadFile::CSV*/);
	QApplication::restoreOverrideCursor();
}

// REFACTOR: almost completely duplicates the above function
void MainWindow::actions_fileExportRetentionData_triggered()
{
	CHECK_PRECOND_RET(m_scope->file() != NULL);

	QString sFilename = QFileDialog::getSaveFileName(
		this,
		tr("Export Retention Data"),
		Globals->lastDir(),
		tr("Comma Separated Values (*.csv)"));

	if (sFilename.isEmpty())
		return;

	QFileInfo fi(sFilename);
	if (fi.suffix().isEmpty())
		sFilename += ".csv";

	QMessageBox msg(QMessageBox::Information, tr("Exporting..."), tr("The retention data is currently being exported."));
	msg.setStandardButtons(QMessageBox::NoButton);
	msg.show();
	// Let window repaint itself before we do the slow process of exporting
	QApplication::processEvents();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m_scope->file()->exportRetentionData(sFilename /*, EadFile::CSV*/);
	QApplication::restoreOverrideCursor();
}

void MainWindow::actions_fileExit_triggered()
{
	close();
}

void MainWindow::on_actHelpAbout_triggered()
{
	QString s;
	s = tr(
			"<h2>%0 %1 (%2)</h2>"
			"<p>Copyright &copy; 2010 Syntech</p>"
			"<p>Data acquisition for Gas Chromatograph with EAD.</p>"
			"<p>For use with the Syntech IDAC series of Data Acquisition Systems.</p>"
			"<p>SYNTECH<br/>Equipment and Research<br/>Hansjakobstrasse 14<br/>79199 KIRCHZARTEN<br/>Germany</p>"
			"<p>More information can be found at:<br/>"
			"<a href='http://www.syntech.nl'>http://www.syntech.nl</a></p>"
			"<p>License: GPL</p>").arg(tr(APPNAME)).arg(APPVERSION).arg(APPVERSIONDATE);

	QMessageBox::about(this, tr(APPNAME), s);
}
