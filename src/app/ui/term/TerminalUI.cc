#include "TerminalUI.h"

#include <QApplication>
#include <QTimer>

#include "Prompt.h"
#include "monitor/Monitor.h"
#include "monitor/MonitorManager.h"
#include "sim/Simulation.h"
#include "utils/misc.h"
#include "ui/gui/MainWindow.h"
#include "ui/SimulationController.h"

using namespace std;


//===========================================================================
// TerminalUI
//===========================================================================

//---------------------------------------------------------------------------
// TerminalUI::TerminalUI
//---------------------------------------------------------------------------
TerminalUI::TerminalUI( SimulationController *_simulationController )
	: simulationController( _simulationController )
	, statusTextMonitor( NULL )
	, gui( NULL )
{
	connectMonitors();

	prompt = new Prompt();

	connect( simulationController, SIGNAL(step()),
			 this, SLOT(step()) );
}

//---------------------------------------------------------------------------
// TerminalUI::~TerminalUI
//---------------------------------------------------------------------------
TerminalUI::~TerminalUI()
{
	if( gui )
		delete gui;
}

//---------------------------------------------------------------------------
// TerminalUI::connectMonitors
//---------------------------------------------------------------------------
void TerminalUI::connectMonitors()
{
	citfor( Monitors, simulationController->getMonitorManager()->getMonitors(), it )
	{
		Monitor *monitor = *it;

		if( monitor->getType() == Monitor::STATUS_TEXT )
		{
			statusTextMonitor = dynamic_cast<StatusTextMonitor *>( monitor );
            statusTextMonitor->update += [=]() {this->status();};
		}
	}
}

//---------------------------------------------------------------------------
// TerminalUI::step
//---------------------------------------------------------------------------
void TerminalUI::step()
{
	char *_cmd = prompt->getUserInput();
	if( _cmd )
	{
		string cmd = _cmd;
		free( _cmd );

		if( cmd == "end" )
		{
			simulationController->end();
		}
		else if( cmd == "gui" )
		{
			if( gui == NULL )
			{
				gui = new MainWindow( simulationController, false );
				connect( gui, SIGNAL(closing()),
						 this, SLOT(guiClosing()) );
				cout << "GUI 已显示，您可能需要将窗口置于前台。" << endl;
			}
		}
		else
		{
			if( cmd != "help" )
			{
				cerr << "无效命令。" << endl;
			}

			cerr << "help - 显示此帮助信息。" << endl;
			cerr << "end - 结束仿真。" << endl;
			cerr << "gui - 显示图形界面。" << endl;
		}
	}
}

//---------------------------------------------------------------------------
// TerminalUI::status
//---------------------------------------------------------------------------
void TerminalUI::status()
{
	if( prompt->isActive() )
		return;

	printf( "------------------------------------------------------------\n" );

	itfor( StatusText, statusTextMonitor->getStatusText(), it )
	{
		printf( "%s\n", *it );
	}
}

//---------------------------------------------------------------------------
// TerminalUI::guiClosing
//---------------------------------------------------------------------------
void TerminalUI::guiClosing()
{
	delete gui;
	gui = NULL;
}
