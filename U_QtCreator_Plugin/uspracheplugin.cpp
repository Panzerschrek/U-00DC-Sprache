#include "uspracheplugin.h"
#include "uspracheconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

namespace USprache {
namespace Internal {

USprachePlugin::USprachePlugin()
{
	// Create your members
}

USprachePlugin::~USprachePlugin()
{
	// Unregister objects from the plugin manager's object pool
	// Delete members
}

bool USprachePlugin::initialize(const QStringList &arguments, QString *errorString)
{
	// Register objects in the plugin manager's object pool
	// Load settings
	// Add actions to menus
	// Connect to other plugins' signals
	// In the initialize function, a plugin can be sure that the plugins it
	// depends on have initialized their members.

	Q_UNUSED(arguments)
	Q_UNUSED(errorString)

	auto action = new QAction(tr("USprache Action"), this);
	Core::Command *cmd = Core::ActionManager::registerAction(action, Constants::ACTION_ID,
															 Core::Context(Core::Constants::C_GLOBAL));
	cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Meta+A")));
	connect(action, &QAction::triggered, this, &USprachePlugin::triggerAction);

	Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
	menu->menu()->setTitle(tr("USprache"));
	menu->addAction(cmd);
	Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

	return true;
}

void USprachePlugin::extensionsInitialized()
{
	// Retrieve objects from the plugin manager's object pool
	// In the extensionsInitialized function, a plugin can be sure that all
	// plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag USprachePlugin::aboutToShutdown()
{
	// Save settings
	// Disconnect from signals that are not needed during shutdown
	// Hide UI (if you add UI that is not in the main window directly)
	return SynchronousShutdown;
}

void USprachePlugin::triggerAction()
{
	QMessageBox::information(Core::ICore::mainWindow(),
							 tr("Action Triggered"),
							 tr("This is an action from USprache."));
}

} // namespace Internal
} // namespace USprache
