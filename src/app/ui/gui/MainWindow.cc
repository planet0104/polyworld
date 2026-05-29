#include "MainWindow.h"

// System
#include <climits>

// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>

// Local
#include "BrainMonitorView.h"
#include "ChartMonitorView.h"
#include "MainWindow.h"
#include "PovMonitorView.h"
#include "SceneMonitorView.h"
#include "StatusTextMonitorView.h"
#include "ToggleWidgetOpenAction.h"
#include "agent/agent.h"
#include "monitor/Monitor.h"
#include "monitor/MonitorManager.h"
#include "sim/globals.h"
#include "sim/Simulation.h"
#include "ui/SimulationController.h"
#include "utils/misc.h"

#if __APPLE__
static QMenuBar *menuBar = NULL;
#endif

//===========================================================================
// MainWindow
//===========================================================================

//---------------------------------------------------------------------------
// MainWindow::MainWindow
//---------------------------------------------------------------------------
MainWindow::MainWindow( SimulationController *_simulationController,
						bool _endOnClose )
	: QMainWindow( 0, 0 )
	, simulationController( _simulationController )
	, endOnClose( _endOnClose )
{
	QApplication::setQuitOnLastWindowClosed( endOnClose );

	setWindowTitle( "Polyworld 人工生命" );
	setMinimumSize(QSize(200, 200));
	loadSettings();

	show();

	createMonitorViews();

#if __APPLE__
	QMenuBar *menuBar = ::menuBar = new QMenuBar(0);
#else
	QMenuBar *menuBar = this->menuBar();
#endif

	addRunMenu( menuBar );
	addViewMenu( menuBar );
}


//---------------------------------------------------------------------------
// MainWindow::~MainWindow
//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
#if __APPLE__
	delete ::menuBar;
#endif

	saveSettings();

	itfor( MonitorViews, monitorViews, it )
	{
		delete *it;
	}
}

//---------------------------------------------------------------------------
// MainWindow::closeEvent
//---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent* ce)
{
	if( endOnClose )
	{
		if( !exitOnUserConfirm() )
		{
			ce->ignore();
		}
	}

	if( ce->isAccepted() )
		emit closing();
}

//---------------------------------------------------------------------------
// MainWindow::createMonitorViews
//---------------------------------------------------------------------------
void MainWindow::createMonitorViews()
{
	citfor( Monitors, simulationController->getMonitorManager()->getMonitors(), it )
	{
		Monitor *_monitor = *it;
		MonitorView *view = NULL;

		switch( _monitor->getType() )
		{
		case Monitor::CHART:
			{
				ChartMonitor *monitor = dynamic_cast<ChartMonitor *>( _monitor );
				view = new ChartMonitorView( monitor );
			}
			break;
		case Monitor::BRAIN:
			{
				BrainMonitor *monitor = dynamic_cast<BrainMonitor *>( _monitor );
				view = new BrainMonitorView( monitor );
			}
			break;
		case Monitor::POV:
			{
				PovMonitor *monitor = dynamic_cast<PovMonitor *>( _monitor );
				view = new PovMonitorView( monitor );
			}
			break;
		case Monitor::STATUS_TEXT:
			{
				StatusTextMonitor *monitor = dynamic_cast<StatusTextMonitor *>( _monitor );
				view = new StatusTextMonitorView( monitor );
			}
			break;
		case Monitor::SCENE:
			{
				SceneMonitor *monitor = dynamic_cast<SceneMonitor *>( _monitor );
				view = new SceneMonitorView( monitor );
			}
			break;
		case Monitor::FARM:
			{
				// no-op
			}
			break;
		default:
			assert( false );
		}

		if( view )
		{
			if( (_monitor->getType() == Monitor::SCENE)
				&& (centralWidget() == NULL) )
			{
				setCentralWidget( view );
			}

			view->loadSettings();

			monitorViews.push_back( view );
		}
	}
}

//---------------------------------------------------------------------------
// MainWindow::addRunMenu
//---------------------------------------------------------------------------
void MainWindow::addRunMenu( QMenuBar *menuBar )
{
	// Run menu
	QMenu* menu = new QMenu( "&运行", this );
	menuBar->addMenu( menu );
	
	menu->addAction( "&暂停/继续", this, SLOT(pauseOrResume()), Qt::CTRL + Qt::Key_P);

	pausedStepAction = new QAction( "单步", this );
	pausedStepAction->setShortcut( Qt::Key_Right );
	pausedStepAction->setEnabled( false );
	connect( pausedStepAction, SIGNAL(triggered()),
			 simulationController, SLOT(pausedStep()) );
	menu->addAction( pausedStepAction );

	menu->addSeparator();

	menu->addAction( "在指定&时间步结束...", this, SLOT(endAtTimestep()));
	menu->addAction( "立即&结束", this, SLOT(endNow()));

#if __APPLE__
	// This is automatically moved over to the application menu.
    menu->addAction( "&退出", this, SLOT(endNow()) );
#endif
}

//---------------------------------------------------------------------------
// MainWindow::addViewMenu
//---------------------------------------------------------------------------
void MainWindow::addViewMenu( QMenuBar *menuBar )
{
	// View menu
	QMenu *menu = new QMenu( "&视图", this );
	menuBar->addMenu( menu );

	itfor( MonitorViews, monitorViews, it )
	{
		MonitorView *view = *it;
		QAction *action = new ToggleWidgetOpenAction( this,
													  view,
													  view->getMonitor()->getName() );
		menu->addAction( action );
	}
}

//---------------------------------------------------------------------------
// MainWindow::pauseOrResume
//---------------------------------------------------------------------------
void MainWindow::pauseOrResume()
{
	if( simulationController->isPaused() )
	{
		simulationController->resume();
	}
	else
	{
		simulationController->pause();
	}

	pausedStepAction->setEnabled( simulationController->isPaused() );
}

//---------------------------------------------------------------------------
// MainWindow::endAtTimestep
//---------------------------------------------------------------------------
void MainWindow::endAtTimestep()
{
	bool promptAgain;
	int defaultValue = simulationController->getSimulation()->GetMaxSteps();

	do
	{
		promptAgain = false;

		bool ok;
		int requestedTimestep =
			QInputDialog::getInt( this,
								  "输入结束时间步",
								  "Polyworld 应在哪个时间步结束？",
								  defaultValue,
								  1,
								  INT_MAX,
								  1,
								  &ok );

		if( ok )
		{
			string result = simulationController->end( requestedTimestep );
			if( result != "" )
			{
				QMessageBox::critical( this,
									   "设置结束时间步失败",
									   result.c_str() );
				promptAgain = true;
				defaultValue = requestedTimestep;
			}
		}
	} while( promptAgain );
}

//---------------------------------------------------------------------------
// MainWindow::endNow
//---------------------------------------------------------------------------
void MainWindow::endNow()
{
	exitOnUserConfirm();
}

//---------------------------------------------------------------------------
// MainWindow::loadSettings
//---------------------------------------------------------------------------
void MainWindow::loadSettings()
{
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;
	settings.beginGroup( "mainWindow" );

	QDesktopWidget* desktop = QApplication::desktop();

	resize( settings.value("w", int(desktop->width() * 0.5)).toInt(),
			settings.value("h", int(desktop->height() * 0.5)).toInt() );

	move( settings.value("x", int(desktop->width() * 0.25)).toInt(),
		  settings.value("y", int(desktop->height() * 0.25)).toInt() );
}

//---------------------------------------------------------------------------
// MainWindow::saveSettings
//---------------------------------------------------------------------------
void MainWindow::saveSettings()
{
	QSettings settings;
	settings.beginGroup( "mainWindow" );
	
	settings.setValue( "x", x() );
	settings.setValue( "y", y() );
	settings.setValue( "w", width() );
	settings.setValue( "h", height() );
}

//---------------------------------------------------------------------------
// MainWindow::exitOnUserConfirm
//---------------------------------------------------------------------------
bool MainWindow::exitOnUserConfirm()
{
	QMessageBox::StandardButton response =
		QMessageBox::question( this,
							   "确认结束 Polyworld",
							   "确定要立即结束 Polyworld 吗？",
							   QMessageBox::Yes | QMessageBox::Cancel,
							   QMessageBox::Yes );

	if( response == QMessageBox::Yes )
	{
		simulationController->end();

		return true;
	}
	else
	{
		return false;
	}
}
