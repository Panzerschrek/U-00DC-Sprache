#pragma once

#include "usprache_global.h"

#include <extensionsystem/iplugin.h>

namespace USprache {
namespace Internal {

class USprachePlugin : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "USprache.json")

public:
	USprachePlugin();
	~USprachePlugin();

	bool initialize(const QStringList &arguments, QString *errorString);
	void extensionsInitialized();
	ShutdownFlag aboutToShutdown();

private:
	void triggerAction();
};

} // namespace Internal
} // namespace USprache
