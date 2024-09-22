#include "AppSettingsImpl.h"

AppSettingsImpl& AppSettingsImpl::instance()
{
	static AppSettingsImpl settings;
	return settings;
}