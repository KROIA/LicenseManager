#pragma once

#include <AppSettings.h>

class AppSettingsImpl : public AppSettings::ApplicationSettings
{
public:
	

	AppSettingsImpl()
		: ApplicationSettings("LicenseManagerSettings")
	{
		setPath("data");

		addGroup(projectGroup);
		addGroup(generalGroup);
	}

public:
	class ProjectGroup : public AppSettings::SettingsGroup
	{
	public:
		ProjectGroup()
			: SettingsGroup("Project")
		{
			addSetting(projectsPath);
			addSetting(lastLoadedProject);
		}

		AppSettings::Setting projectsPath = AppSettings::Setting("projectsPath", "data/projects");
		AppSettings::Setting lastLoadedProject = AppSettings::Setting("lastLoadedProject", "");
	};
	class General : public AppSettings::SettingsGroup
	{
	public:
		General()
			: SettingsGroup("General")
		{
			addSetting(showConsole);
		}

		AppSettings::Setting showConsole = AppSettings::Setting("showConsole", false);
	};

	static AppSettingsImpl& instance();


	static ProjectGroup& getProjectGroup()
	{
		return instance().projectGroup;
	}
	static General& getGeneralGroup()
	{
		return instance().generalGroup;
	}
	static void save()
	{
		instance().AppSettings::ApplicationSettings::save();
	}
	static void load()
	{
		instance().AppSettings::ApplicationSettings::load();
	}
private:
	
	ProjectGroup projectGroup;
	General generalGroup;
};