#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QMap>
#include "MaterialTabWidget.h"
#include "ConfigModManager.h"
#include "ModCardModel.h"
#include "ModCardWidget.h"
#include "AntScrollArea.h"
#include "NoDataWidget.h"
#include "ModInfo.h"
#include "ApplicationController.h"

class QVBoxLayout;

class ModManagerPage : public QWidget
{
	Q_OBJECT

public:
	explicit ModManagerPage(QSharedPointer<ApplicationController> appController, QWidget* parent = nullptr);
	~ModManagerPage();

	void addDialog(DialogViewController* dialog);

protected:
	void showEvent(QShowEvent* event) override;

	void createExampleMod();

private slots:
	void onModLoaded(const ModInfo& info);
	void onModUnloaded(const QString& modId);
	void onModEnabled(const QString& modId);
	void onModDisabled(const QString& modId);
	void onAllModsLoaded();

	void onToggleClicked(bool enabled);
	void onOpenFolderClicked();
	void onUpdateClicked();
	void onUninstallClicked();

private:
	void initUI();
	void initConnections();
	void loadMods();
	void refreshTabs();
	void createModTab(const QString& modId, QSharedPointer<ModCardModel> model);
	void removeModTab(const QString& modId);
	void updateTabName(const QString& modId);
	void updateModTab(const QString& modId);

	QSharedPointer<ApplicationController> m_appController;
	QSharedPointer<ConfigModManager> m_modManager;
	MaterialTabWidget* m_tabWidget = nullptr;
	NoDataWidget* m_noDataWidget = nullptr;
	QVBoxLayout* m_mainLayout = nullptr;

	QMap<QString, QSharedPointer<ModCardModel>> m_modModels;
	QMap<QString, ModCardWidget*> m_modTabs;

	// 添加映射来跟踪标签页和模组的关联
	QMap<QString, int> m_modTabIndexes; // modId -> tab index
	QMap<int, QString> m_tabIndexMods;  // tab index -> modId

	DialogViewController* m_dialogView;
};