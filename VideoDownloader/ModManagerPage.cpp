#include "ModManagerPage.h"

#include <QDir>
#include <QDesktopServices>
#include <QCoreApplication>

#include "MaterialTabWidget.h"
#include "AntScrollArea.h"
#include "NoDataWidget.h"
#include "NotificationManager.h"
#include "PlatformAggregatorService.h"
#include "ApplicationController.h"
#include "ConfigModManager.h"
#include "ModCardModel.h"
#include "ModCardWidget.h"
#include "ModInfo.h"
#include "Instrumentor.h"

ModManagerPage::ModManagerPage(QSharedPointer<ApplicationController> appController, BubbleViewController* bubbleView, DialogViewController* dialogView, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
	, m_bubbleView(bubbleView)
	, m_dialogView(dialogView)
	, m_modManager(appController->getConfigModManager())
{
	BENCHMARKING_FUNCTION();
	setObjectName("ModManagerPage");
	initUI();
	initConnections();

	// 如果ModManager已经初始化，直接加载模组
	if (m_modManager && m_modManager->isInitialized()) {
		loadMods();
	}
}

ModManagerPage::~ModManagerPage()
{
}

void ModManagerPage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	// 每次显示页面时刷新模组列表
	if (m_modManager && m_modManager->isInitialized()) {
		// 只在需要时重新加载
		if (m_modTabs.isEmpty()) {
			loadMods();
		}
	}
}

// 添加创建示例模组的方法
void ModManagerPage::createExampleMod()
{
	// 创建示例模组信息
	ModInfo exampleInfo;
	exampleInfo.modId = "example_mod";
	exampleInfo.name = "示例模组";
	exampleInfo.version = "1.0.0";
	exampleInfo.author = "系统示例";
	exampleInfo.description = "这是一个示例模组，用于展示模组卡片的外观和功能。您可以通过这个示例了解模组管理界面的使用方法。";
	exampleInfo.enabled = true;
	exampleInfo.priority = 1;
	exampleInfo.loadTime = QDateTime::currentDateTime();
	exampleInfo.modPath = QCoreApplication::applicationDirPath() + "/mods/example_mod";

	// 设置URL模式
	exampleInfo.urlPatterns = QStringList() << "https://example.com/video/.*";

	// 创建配置
	QJsonObject config;
	config["apiEndpoint"] = "https://api.example.com/video";
	exampleInfo.config = config;

	// 创建模组卡片模型
	QSharedPointer<ModCardModel> exampleModel = QSharedPointer<ModCardModel>::create(exampleInfo);

	// 添加到管理器中
	createModTab("example_mod", exampleModel);
}

void ModManagerPage::onModLoaded(const ModInfo& info)
{
	createModTab(info.modId, QSharedPointer<ModCardModel>::create(info));
	refreshTabs();
}

void ModManagerPage::onModUnloaded(const QString& modId)
{
	removeModTab(modId);
	refreshTabs();
}

void ModManagerPage::onModEnabled(const QString& modId)
{
	if (m_modModels.contains(modId)) {
		m_modModels[modId]->setEnabled(true);
	}
	updateModTab(modId);
}

void ModManagerPage::onModDisabled(const QString& modId)
{
	if (m_modModels.contains(modId)) {
		m_modModels[modId]->setEnabled(false);
	}
	updateModTab(modId);
}

void ModManagerPage::onAllModsLoaded()
{
	refreshTabs();
}

void ModManagerPage::onToggleClicked(bool enabled)
{
	// 获取发送信号的ModCardWidget
	ModCardWidget* senderWidget = qobject_cast<ModCardWidget*>(sender());
	if (!senderWidget) return;

	QString modId = senderWidget->model()->modId();

	// 如果是示例模组，只更新UI状态
	if (modId == "example_mod") {
		if (enabled) {
			NotificationManager::instance()->showNotification("示例模组已启用（演示功能）");
		}
		else {
			NotificationManager::instance()->showNotification("示例模组已禁用（演示功能）");
		}
		return;
	}

	if (enabled) {
		if (m_modManager->enableMod(modId)) {
			NotificationManager::instance()->showNotification("模组已启用: " + senderWidget->model()->name());
		}
		else {
			NotificationManager::instance()->showNotification("启用模组失败: " + m_modManager->getLastError());
			// 回滚UI状态
			senderWidget->model()->setEnabled(false);
		}
	}
	else {
		if (m_modManager->disableMod(modId)) {
			NotificationManager::instance()->showNotification("模组已禁用: " + senderWidget->model()->name());
		}
		else {
			NotificationManager::instance()->showNotification("禁用模组失败: " + m_modManager->getLastError());
			// 回滚UI状态
			senderWidget->model()->setEnabled(true);
		}
	}
}

void ModManagerPage::onOpenFolderClicked()
{
	ModCardWidget* senderWidget = qobject_cast<ModCardWidget*>(sender());
	if (!senderWidget) return;

	QString modId = senderWidget->model()->modId();

	// 如果是示例模组，显示提示信息
	if (modId == "example_mod") {
		NotificationManager::instance()->showNotification("这是示例模组，没有真实的文件夹路径");
		return;
	}

	QString modPath = senderWidget->model()->modPath();
	if (!modPath.isEmpty()) {
		QDir modDir(modPath);
		if (modDir.exists()) {
			QUrl url = QUrl::fromLocalFile(modPath);
			QDesktopServices::openUrl(url);
		}
		else {
			NotificationManager::instance()->showNotification("模组目录不存在: " + modPath);
		}
	}
}

void ModManagerPage::onUpdateClicked()
{
	ModCardWidget* senderWidget = qobject_cast<ModCardWidget*>(sender());
	if (!senderWidget) return;

	QString modId = senderWidget->model()->modId();

	// 如果是示例模组，显示提示信息
	if (modId == "example_mod") {
		NotificationManager::instance()->showNotification("示例模组不支持更新功能");
		return;
	}

	NotificationManager::instance()->showNotification("开始更新模组: " + senderWidget->model()->name());

	// TODO: 实现模组更新逻辑
	// 这里可以添加实际的模组更新代码
	/*if (m_modManager->refreshMod(modId)) {
		NotificationManager::instance()->showNotification("模组更新成功: " + senderWidget->model()->name());
	}
	else {
		NotificationManager::instance()->showNotification("模组更新失败");
	}*/
}

void ModManagerPage::onUninstallClicked()
{
	ModCardWidget* senderWidget = qobject_cast<ModCardWidget*>(sender());
	if (!senderWidget) return;

	QString modId = senderWidget->model()->modId();

	// 如果是示例模组，显示提示信息
	if (modId == "example_mod") {
		NotificationManager::instance()->showNotification("示例模组无法卸载");
		return;
	}

	QString modName = senderWidget->model()->name();

	//QMessageBox::StandardButton reply;
	//reply = QMessageBox::question(this, "确认卸载",
	//	QString("确定要卸载模组 \"%1\" 吗？").arg(modName),
	//	QMessageBox::Yes | QMessageBox::No);

	//if (reply == QMessageBox::Yes) {
	//	if (m_modManager->unloadMod(modId)) {
	//		NotificationManager::instance()->showNotification("模组已卸载: " + modName);
	//		// 卸载后从UI中移除
	//		removeModTab(modId);
	//		refreshTabs();
	//	}
	//	else {
	//		NotificationManager::instance()->showNotification("卸载模组失败: " + m_modManager->getLastError());
	//	}
	//}
}

void ModManagerPage::initUI()
{
	BENCHMARKING_FUNCTION();
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(0);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);

	// 创建标签页组件
	m_tabWidget = new MaterialTabWidget(this);
	m_tabWidget->getLayout()->setContentsMargins(15, 0, 5, 0);

	m_mainLayout->addWidget(m_tabWidget);
}

void ModManagerPage::initConnections()
{
	BENCHMARKING_FUNCTION();
	if (m_modManager) {
		connect(m_modManager.get(), &ConfigModManager::modLoaded, this, &ModManagerPage::onModLoaded);
		connect(m_modManager.get(), &ConfigModManager::modUnloaded, this, &ModManagerPage::onModUnloaded);
		connect(m_modManager.get(), &ConfigModManager::modEnabled, this, &ModManagerPage::onModEnabled);
		connect(m_modManager.get(), &ConfigModManager::modDisabled, this, &ModManagerPage::onModDisabled);
		//connect(m_modManager.get(), &ConfigModManager::allModsLoaded, this, &ModManagerPage::onAllModsLoaded);
	}
}

void ModManagerPage::loadMods()
{
	BENCHMARKING_FUNCTION();
	if (!m_modManager) return;

	// 清空现有模组
	m_modModels.clear();
	m_modTabs.clear();
	m_modTabIndexes.clear();
	m_tabIndexMods.clear();

	// 获取所有已加载的模组
	QList<QString> modIds = m_modManager->getLoadedMods();

	if (modIds.isEmpty()) {
		// 如果没有真实模组，创建示例模组
		createExampleMod();
	}
	else {
		for (const QString& modId : modIds) {
			ModInfo modInfo = m_modManager->getMod(modId);
			if (modInfo.isValid()) {
				createModTab(modId, QSharedPointer<ModCardModel>::create(modInfo));
			}
		}
	}

	refreshTabs();
}

void ModManagerPage::refreshTabs()
{
	BENCHMARKING_FUNCTION();
	if (!m_tabWidget) return;

	// 清空所有标签页
	while (m_tabWidget->count() > 0) {
		m_tabWidget->removeTab(0);
	}

	// 清空映射
	m_modTabIndexes.clear();
	m_tabIndexMods.clear();

	// 如果有模组，添加模组标签页
	if (!m_modTabs.isEmpty()) {
		int tabIndex = 0;
		for (auto it = m_modTabs.begin(); it != m_modTabs.end(); ++it) {
			const QString& modId = it.key();
			ModCardWidget* cardWidget = it.value();

			if (m_modModels.contains(modId)) {
				QString tabName = m_modModels[modId]->name();
				if (tabName.isEmpty()) {
					tabName = modId;
				}

				// 创建滚动区域包装卡片，确保内容可滚动
				AntScrollArea* scrollArea = new AntScrollArea(AntScrollArea::ScrollVertical, this);
				QWidget* scrollContent = new QWidget(this);
				QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
				scrollLayout->setSpacing(0);
				scrollLayout->setContentsMargins(20, 20, 20, 20);
				scrollLayout->addWidget(cardWidget);
				scrollLayout->addStretch();

				scrollArea->addWidget(scrollContent);
				m_tabWidget->addTab(scrollArea, tabName);

				// 记录映射关系
				m_modTabIndexes[modId] = tabIndex;
				m_tabIndexMods[tabIndex] = modId;
				tabIndex++;
			}
		}
	}
	else {
		// 没有模组时显示无数据页面
		m_noDataWidget = new NoDataWidget(this);
		m_noDataWidget->setText(tr("暂无模组"));
		m_tabWidget->addTab(m_noDataWidget, tr("模组管理"));
	}
}

void ModManagerPage::createModTab(const QString& modId, QSharedPointer<ModCardModel> model)
{
	BENCHMARKING_FUNCTION();
	if (m_modTabs.contains(modId)) {
		// 如果已存在，更新现有卡片
		m_modTabs[modId]->setModel(model);

		// 更新标签页名称
		updateTabName(modId);
	}
	else {
		// 创建新的卡片组件
		ModCardWidget* cardWidget = new ModCardWidget(m_appController->getPlatformService()->getPlatform(modId), model, m_bubbleView, m_dialogView, this);
		connect(cardWidget, &ModCardWidget::toggleClicked,
			this, &ModManagerPage::onToggleClicked);
		connect(cardWidget, &ModCardWidget::openFolderClicked,
			this, &ModManagerPage::onOpenFolderClicked);
		connect(cardWidget, &ModCardWidget::updateClicked,
			this, &ModManagerPage::onUpdateClicked);
		connect(cardWidget, &ModCardWidget::uninstallClicked,
			this, &ModManagerPage::onUninstallClicked);

		m_modTabs[modId] = cardWidget;
		m_modModels[modId] = model;
	}
}

void ModManagerPage::removeModTab(const QString& modId)
{
	BENCHMARKING_FUNCTION();
	if (m_modTabs.contains(modId)) {
		ModCardWidget* cardWidget = m_modTabs.take(modId);
		cardWidget->deleteLater();
	}

	if (m_modModels.contains(modId)) {
		m_modModels.remove(modId);
	}

	// 从映射中移除
	if (m_modTabIndexes.contains(modId)) {
		int tabIndex = m_modTabIndexes[modId];
		m_modTabIndexes.remove(modId);
		m_tabIndexMods.remove(tabIndex);
	}
}

void ModManagerPage::updateTabName(const QString& modId)
{
	BENCHMARKING_FUNCTION();
	if (!m_modModels.contains(modId) || !m_modTabIndexes.contains(modId)) {
		return;
	}

	QString newName = m_modModels[modId]->name();
	if (newName.isEmpty()) {
		newName = modId;
	}

	int tabIndex = m_modTabIndexes[modId];
	// 这里我们需要找到对应标签页并更新名称
	// 由于MaterialTabWidget没有提供直接更新标签名称的方法，
	// 我们需要在refreshTabs中处理
}

void ModManagerPage::updateModTab(const QString& modId)
{
	BENCHMARKING_FUNCTION();
	if (m_modModels.contains(modId) && m_modTabs.contains(modId)) {
		// 更新模型数据
		ModInfo modInfo = m_modManager->getMod(modId);
		if (modInfo.isValid()) {
			m_modModels[modId]->fromModInfo(modInfo);
		}
	}
}