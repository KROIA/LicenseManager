#include "AppSettingsImpl.h"

AppSettingsImpl& AppSettingsImpl::instance()
{
	static AppSettingsImpl settings;
	static bool loaded = false;
	if (!loaded)
	{
		settings.AppSettings::ApplicationSettings::load();
		loaded = true;
	}
	return settings;
}