#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QMap>
#include "MaterialTabWidget.h"
#include "ModManager.h"
#include "ModCardModel.h"
#include "ModCardWidget.h"
#include "AntScrollArea.h"
#include "NoDataWidget.h"

class QVBoxLayout;

class ModManagerPage : public QWidget
{
	Q_OBJECT

public:
	explicit ModManagerPage(QSharedPointer<ModManager> modManager, QWidget* parent = nullptr);
	~ModManagerPage();

protected:
	void showEvent(QShowEvent* event) override;

	void createExampleMod();

private slots:
	void onModLoaded(const QString& modId, const ModManager::ModInfo& info);
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
	void updateModTab(const QString& modId);

	QSharedPointer<ModManager> m_modManager;
	MaterialTabWidget* m_tabWidget = nullptr;
	NoDataWidget* m_noDataWidget = nullptr;
	QVBoxLayout* m_mainLayout = nullptr;

	QMap<QString, QSharedPointer<ModCardModel>> m_modModels;
	QMap<QString, ModCardWidget*> m_modTabs;
};