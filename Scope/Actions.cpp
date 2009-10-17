/**
 * Copyright (C) 2008,2009  Ellis Whitehead
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

#include "Actions.h"

#include <QActionGroup>


#define MAX_RECENT_FILES 10


Actions::Actions(QObject *parent)
	: QObject(parent)
{
	fileNew = new QAction(tr("&New"), this);
	fileNew->setShortcut(tr("Ctrl+N"));

	fileOpen = new QAction(tr("&Open"), this);
	fileOpen->setShortcut(tr("Ctrl+O"));

	for (int i = 0; i < MAX_RECENT_FILES; i++)
	{
		QAction* act = new QAction(QString(), this);
		act->setVisible(false);
		act->setData(i);
		fileOpenRecentActions << act;
	}

	fileSave = new QAction(tr("&Save"), this);
	fileSave->setShortcut(tr("Ctrl+S"));

	fileSaveAs = new QAction(tr("Save &As"), this);

	fileComment = new QAction(tr("Edit File &Comment..."), this);
	fileComment->setIcon(QIcon(":/images/comment-24x24.png"));
	fileComment->setIconText(tr("Comment"));
	fileComment->setToolTip(tr("Edit the general comment for the whole file"));

	fileExportSignalData = new QAction(tr("&Export Signal Data..."), this);

	fileExportRetentionData = new QAction(tr("&Export Retention Data..."), this);

	fileLoadSampleProject = new QAction(tr("&Load Sample Project..."), this);

	fileExit = new QAction(tr("E&xit"), this);
	fileExit->setShortcut(tr("Alt+F4"));

	//
	// View Menu
	//

	QActionGroup* group;
	
	group = new QActionGroup(this);
	viewViewMode = new QAction(tr("&View Mode"), group);
	viewViewMode->setCheckable(true);
	viewViewMode->setChecked(true);
	viewViewMode->setShortcut(tr("Ctrl+V"));
	viewPublishMode = new QAction(tr("&Publish Mode"), group);
	viewPublishMode->setCheckable(true);
	viewPublishMode->setShortcut(tr("Ctrl+P"));

	group = new QActionGroup(this);
	viewChartAverages = new QAction(tr("Chart: Averages"), group);
	viewChartAverages->setCheckable(true);
	viewChartAverages->setChecked(true);
	viewChartAverages->setShortcut(tr("Ctrl+1"));
	viewChartEads = new QAction(tr("Chart: EADs"), group);
	viewChartEads->setCheckable(true);
	viewChartEads->setShortcut(tr("Ctrl+2"));
	viewChartFids = new QAction(tr("Chart: FIDs"), group);
	viewChartFids->setCheckable(true);
	viewChartFids->setShortcut(tr("Ctrl+3"));
	viewChartAll = new QAction(tr("Chart: All"), group);
	viewChartAll->setCheckable(true);
	viewChartAll->setShortcut(tr("Ctrl+4"));
	viewChartRecording = new QAction(tr("Chart: Recording"), group);
	viewChartRecording->setCheckable(true);
	viewChartRecording->setEnabled(false);
	viewChartRecording->setShortcut(tr("Ctrl+5"));

	viewZoomIn = new QAction(tr("Zoom In"), this);
	viewZoomIn->setShortcut(tr("Ctrl++"));
	viewZoomIn->setIcon(QIcon(":/images/stock_zoom-in.png"));
	viewZoomOut = new QAction(tr("Zoom Out"), this);
	viewZoomOut->setShortcut(tr("Ctrl+-"));
	viewZoomOut->setIcon(QIcon(":/images/stock_zoom-out.png"));
	viewZoomFull = new QAction(tr("Zoom Full"), this);
	viewZoomFull->setShortcut(tr("Ctrl+="));
	viewZoomFull->setIcon(QIcon(":/images/stock_zoom-page-width.png"));

	viewWaveComments = new QAction(tr("Show wave &comments"), this);
	viewWaveComments->setCheckable(true);

	group = new QActionGroup(this);
	
	viewHidePeaks = new QAction(tr("&Hide Peaks"), group);
	viewHidePeaks->setCheckable(true);
	viewHidePeaks->setIconText(tr("Hide"));
	viewHidePeaks->setToolTip(tr("Hide peaks"));

	viewDetectedPeaks = new QAction(tr("Show &Detected Peaks"), group);
	viewDetectedPeaks->setCheckable(true);
	viewDetectedPeaks->setIconText(tr("All"));
	viewDetectedPeaks->setToolTip(tr("Show both verified and automatically detected peaks"));

	viewVerifiedPeaks = new QAction(tr("Show &Verified Peaks"), group);
	viewVerifiedPeaks->setCheckable(true);
	viewVerifiedPeaks->setChecked(true);
	viewVerifiedPeaks->setIconText(tr("Verified"));
	viewVerifiedPeaks->setToolTip(tr("Show the peaks which you have manually verified"));

	viewEditPeaks = new QAction(tr("&Edit Peaks"), group);
	viewEditPeaks->setCheckable(true);
	viewEditPeaks->setIconText(tr("Edit"));
	viewEditPeaks->setToolTip(tr("Active the mode for selecting peaks and adjusting their areas"));

	//
	// Record menu
	//

	recordRecord = new QAction(tr("&Record..."), this);
	recordRecord->setIcon(QIcon(":/images/record-24x24.png"));
	recordRecord->setShortcut(tr("Ctrl+R"));

	recordHardwareSettings = new QAction(tr("&Hardware Settings..."), this);
	//recordOptions->setIconText("Options");

	recordSave = new QAction(tr("&Stop and Save"), this);
	
	recordDiscard = new QAction(tr("&Discard"), this);

	recordHardwareConnect = new QAction(tr("&Connect to Hardware"), this);
}
