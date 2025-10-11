#include "HomePage.h"
#include <QLayout>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QLabel>
#include <QApplication>
#include <QScreen>
#include "DesignSystem.h"
#include "StyleSheet.h"
#include "AntButton.h"

HomePage::HomePage(QSharedPointer<ApplicationController> appController, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
{
	setObjectName("HomePage");
	setupUI();
	setupConnections();
	setupSearchResultsContainer();

	// 初始隐藏搜索结果容器
	m_searchResultsContainer->hide();

	// 连接主题切换信号
	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, &HomePage::updateSearchResultsStyle);
}

HomePage::~HomePage()
{
}

void HomePage::setupUI()
{
	// 创建搜索输入框
	QStringList listItems = {};
	antInput = new AntInput(300, listItems);
	antInput->setFixedWidth(400);
	antInput->setFixedHeight(50);
	antInput->setPlaceholderText("搜索视频内容...");

	// 使用网格布局实现居中
	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setSpacing(0);

	// 将输入框放在网格的中心位置
	gridLayout->addWidget(antInput, 1, 1);

	// 设置行列的伸缩因子，使输入框位于高度的1/4处
	gridLayout->setRowStretch(0, 1);  // 顶部弹性空间 - 占1份
	gridLayout->setRowStretch(1, 0);  // 中间行（固定高度）- 不拉伸
	gridLayout->setRowStretch(2, 3);  // 底部弹性空间 - 占3份，这样输入框就在1/(1+3)=1/4位置
	gridLayout->setColumnStretch(0, 1); // 左侧弹性空间
	gridLayout->setColumnStretch(1, 0); // 中间列（固定宽度）
	gridLayout->setColumnStretch(2, 1); // 右侧弹性空间
}

void HomePage::setupConnections()
{
	connect(antInput, &AntInput::textChanged, this, &HomePage::onSearchTextChanged);
	connect(antInput, &AntInput::returnPressed, this, &HomePage::onSearchClicked);
}

void HomePage::setupSearchResultsContainer()
{
	// 创建搜索结果容器
	m_searchResultsContainer = new QWidget(this);
	m_searchResultsContainer->setObjectName("SearchResultsContainer");

	QVBoxLayout* containerLayout = new QVBoxLayout(m_searchResultsContainer);
	containerLayout->setContentsMargins(16, 16, 16, 16);
	containerLayout->setSpacing(12);

	// 已选择数量标签
	m_selectedCountLabel = new QLabel("已选择 0/0", m_searchResultsContainer);

	// 搜索结果列表
	m_searchResultsList = new QListWidget(m_searchResultsContainer);
	m_searchResultsList->setObjectName("SearchResultsList");
	m_searchResultsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_searchResultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// 底部操作栏
	QHBoxLayout* bottomLayout = new QHBoxLayout();
	bottomLayout->setContentsMargins(0, 0, 0, 0);
	bottomLayout->setSpacing(12);

	// 全选复选框
	m_selectAllCheckBox = new QCheckBox("全选", m_searchResultsContainer);

	// 下一步按钮
	m_nextButton = new AntButton("下一步", 12, m_searchResultsContainer);
	m_nextButton->setFixedSize(100, 36);

	bottomLayout->addWidget(m_selectAllCheckBox);
	bottomLayout->addStretch();
	bottomLayout->addWidget(m_nextButton);

	// 组装容器布局
	containerLayout->addWidget(m_selectedCountLabel);
	containerLayout->addWidget(m_searchResultsList, 1); // 列表占据剩余空间
	containerLayout->addLayout(bottomLayout);

	// 连接信号槽
	connect(m_selectAllCheckBox, &QCheckBox::checkStateChanged,
		this, &HomePage::onSelectAllStateChanged);
	connect(m_nextButton, &AntButton::clicked,
		this, &HomePage::onNextButtonClicked);

	// 初始化样式
	updateSearchResultsStyle();
}

void HomePage::onSearchTextChanged(const QString& text)
{
	// 搜索文本变化时，可以添加搜索建议等功能
	Q_UNUSED(text);
}

void HomePage::onSearchClicked()
{
	QString searchText = antInput->text().trimmed();
	if (searchText.isEmpty()) {
		m_searchResultsContainer->hide();
		return;
	}

	// 清空之前的结果和选择
	m_searchResultsList->clear();
	clearAllSelections();

	// 模拟搜索数据
	loadMockSearchData();

	// 在数据加载完成后更新选择计数
	updateSelectedCount();

	// 显示并定位搜索结果容器
	updateSearchResultsPosition();
	m_searchResultsContainer->show();
	m_searchResultsContainer->raise();
}

void HomePage::onSelectAllStateChanged(int state)
{
	// 阻塞列表项的信号，避免触发单个项的状态改变
	blockItemSignals(true);

	// 遍历所有列表项，更新复选框状态
	for (int i = 0; i < m_searchResultsList->count(); ++i) {
		QListWidgetItem* item = m_searchResultsList->item(i);
		QWidget* itemWidget = m_searchResultsList->itemWidget(item);
		if (itemWidget) {
			QCheckBox* checkbox = itemWidget->findChild<QCheckBox*>();
			if (checkbox) {
				checkbox->setCheckState(static_cast<Qt::CheckState>(state));
			}
		}
	}

	// 更新选择计数
	m_selectedIndexes.clear();
	if (state == Qt::Checked) {
		for (int i = 0; i < m_totalItems; i++) {
			m_selectedIndexes.append(i);
		}
	}

	updateSelectedCount();

	// 解除阻塞
	blockItemSignals(false);
}

void HomePage::onItemCheckboxStateChanged(int state)
{
	QCheckBox* checkbox = qobject_cast<QCheckBox*>(sender());
	if (!checkbox) return;

	// 获取项的索引
	int index = checkbox->property("itemIndex").toInt();

	if (state == Qt::Checked) {
		if (!m_selectedIndexes.contains(index)) {
			m_selectedIndexes.append(index);
		}
	}
	else {
		m_selectedIndexes.removeAll(index);
	}

	updateSelectedCount();

	// 更新全选框状态
	bool allSelected = (m_selectedIndexes.count() == m_totalItems);
	bool anySelected = !m_selectedIndexes.isEmpty();

	m_selectAllCheckBox->blockSignals(true);
	if (allSelected) {
		m_selectAllCheckBox->setCheckState(Qt::Checked);
	}
	else if (anySelected) {
		m_selectAllCheckBox->setCheckState(Qt::PartiallyChecked);
	}
	else {
		m_selectAllCheckBox->setCheckState(Qt::Unchecked);
	}
	m_selectAllCheckBox->blockSignals(false);
}

void HomePage::onNextButtonClicked()
{
	if (m_selectedIndexes.isEmpty()) {
		// 可以显示提示信息
		return;
	}

	navigateToDownloadQueue();
}

void HomePage::updateSelectedCount()
{
	int selectedCount = m_selectedIndexes.size();
	m_selectedCountLabel->setText(QString("已选择 %1/%2").arg(selectedCount).arg(m_totalItems));

	// 根据是否有选中项更新下一步按钮状态
	m_nextButton->setEnabled(selectedCount > 0);
}

void HomePage::addSearchResultItem(const QString& title, const QString& duration, const QString& author)
{
	QListWidgetItem* item = new QListWidgetItem(m_searchResultsList);

	// 创建自定义的列表项widget
	QWidget* itemWidget = new QWidget();
	itemWidget->setObjectName("SearchResultItem");
	QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
	itemLayout->setContentsMargins(12, 8, 12, 8);
	itemLayout->setSpacing(12);

	// 复选框
	QCheckBox* checkbox = new QCheckBox();
	checkbox->setFixedSize(16, 16);
	checkbox->setProperty("itemIndex", m_totalItems);
	connect(checkbox, &QCheckBox::checkStateChanged, this, &HomePage::onItemCheckboxStateChanged);

	// 视频信息
	QWidget* infoWidget = new QWidget();
	QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
	infoLayout->setContentsMargins(0, 0, 0, 0);
	infoLayout->setSpacing(4);

	QLabel* titleLabel = new QLabel(title);
	titleLabel->setObjectName("SearchResultTitle");

	QLabel* metaLabel = new QLabel(QString("%1 · %2").arg(duration).arg(author));
	metaLabel->setObjectName("SearchResultMeta");

	infoLayout->addWidget(titleLabel);
	infoLayout->addWidget(metaLabel);

	itemLayout->addWidget(checkbox);
	itemLayout->addWidget(infoWidget, 1);
	itemLayout->addStretch();

	// 设置列表项
	item->setSizeHint(QSize(0, 60)); // 固定高度
	m_searchResultsList->setItemWidget(item, itemWidget);

	m_totalItems++;

	// 更新该项的样式
	updateItemStyle(itemWidget);
}

void HomePage::navigateToDownloadQueue()
{
	// 隐藏搜索结果容器
	m_searchResultsContainer->hide();

	// 清空搜索框
	antInput->clear();

	// 发出导航信号
	emit navigateToDownloadRequested();

	qDebug() << "导航到下载队列页面，选中了" << m_selectedIndexes.size() << "个项目";
}

void HomePage::updateSearchResultsPosition()
{
	if (!m_searchResultsContainer || !antInput) return;

	// 获取搜索框的全局位置
	QPoint inputGlobalPos = antInput->mapToGlobal(QPoint(0, 0));
	// 转换为相对于HomePage的位置
	QPoint inputLocalPos = mapFromGlobal(inputGlobalPos);

	// 设置搜索结果容器的位置和大小
	int containerWidth = antInput->width();
	int containerHeight = static_cast<int>(height() * 0.6); // 高度的60%

	m_searchResultsContainer->setFixedSize(containerWidth, containerHeight);
	m_searchResultsContainer->move(inputLocalPos.x(),
		inputLocalPos.y() + antInput->height() + 8);
}

void HomePage::clearAllSelections()
{
	m_selectedIndexes.clear();
	m_totalItems = 0;
	m_selectAllCheckBox->setCheckState(Qt::Unchecked);
	updateSelectedCount();
}

void HomePage::loadMockSearchData()
{
	// 模拟搜索数据
	QStringList mockTitles = {
		"Qt编程入门教程 - 从零开始学习GUI开发",
		"C++高级编程技巧与最佳实践",
		"设计模式在Qt中的应用实例",
		"多线程编程实战指南",
		"现代C++新特性详解"
	};

	QStringList mockDurations = { "12:34", "23:45", "45:12", "34:56", "56:78" };
	QStringList mockAuthors = { "程序员老王", "技术达人", "代码艺术家", "架构师之路", "编程思维" };

	for (int i = 0; i < mockTitles.size(); ++i) {
		addSearchResultItem(mockTitles[i], mockDurations[i], mockAuthors[i]);
	}
}

void HomePage::updateSearchResultsStyle()
{
	// 更新搜索结果容器样式
	m_searchResultsContainer->setStyleSheet(
		QString("#SearchResultsContainer{"
			"background-color: %1;"
			"border: 1px solid %2;"
			"border-radius: 8px;"
			"}")
		.arg(DesignSystem::instance()->currentTheme().cardBackgroundColor.name())
		.arg(DesignSystem::instance()->borderColor().name()));

	// 更新已选择数量标签样式
	m_selectedCountLabel->setStyleSheet(
		QString("QLabel{"
			"font-size: 14px;"
			"color: %1;"
			"font-weight: bold;"
			"}")
		.arg(DesignSystem::instance()->primaryColor().name()));

	// 更新列表样式
	m_searchResultsList->setStyleSheet(
		QString("#SearchResultsList{"
			"background-color: transparent;"
			"border: none;"
			"outline: none;"
			"}"));

	// 更新全选复选框样式
	m_selectAllCheckBox->setStyleSheet(
		QString("QCheckBox{"
			"font-size: 13px;"
			"color: %1;"
			"}"
			"QCheckBox::indicator{"
			"width: 16px;"
			"height: 16px;"
			"}"
			"QCheckBox::indicator:unchecked{"
			"border: 1px solid %2;"
			"background-color: %3;"
			"}"
			"QCheckBox::indicator:checked{"
			"border: 1px solid %4;"
			"background-color: %4;"
			"}"
			"QCheckBox::indicator:indeterminate{"
			"border: 1px solid %4;"
			"background-color: %4;"
			"}")
		.arg(DesignSystem::instance()->primaryTextColor().name())
		.arg(DesignSystem::instance()->borderColor().name())
		.arg(DesignSystem::instance()->cardBackgroundColor().name())
		.arg(DesignSystem::instance()->primaryColor().name()));

	// 更新所有列表项样式
	for (int i = 0; i < m_searchResultsList->count(); ++i) {
		QListWidgetItem* item = m_searchResultsList->item(i);
		QWidget* itemWidget = m_searchResultsList->itemWidget(item);
		if (itemWidget) {
			updateItemStyle(itemWidget);
		}
	}
}

void HomePage::updateItemStyle(QWidget* itemWidget)
{
	if (!itemWidget) return;

	// 更新复选框样式
	QCheckBox* checkbox = itemWidget->findChild<QCheckBox*>();
	if (checkbox) {
		checkbox->setStyleSheet(
			QString("QCheckBox::indicator{"
				"width: 16px;"
				"height: 16px;"
				"}"
				"QCheckBox::indicator:unchecked{"
				"border: 1px solid %1;"
				"background-color: %2;"
				"}"
				"QCheckBox::indicator:checked{"
				"border: 1px solid %3;"
				"background-color: %3;"
				"}")
			.arg(DesignSystem::instance()->borderColor().name())
			.arg(DesignSystem::instance()->cardBackgroundColor().name())
			.arg(DesignSystem::instance()->primaryColor().name()));
	}

	// 更新标题标签样式
	QLabel* titleLabel = itemWidget->findChild<QLabel*>("SearchResultTitle");
	if (titleLabel) {
		titleLabel->setStyleSheet(
			QString("QLabel{"
				"font-size: 14px;"
				"color: %1;"
				"font-weight: bold;"
				"}")
			.arg(DesignSystem::instance()->primaryTextColor().name()));
	}

	// 更新元数据标签样式
	QLabel* metaLabel = itemWidget->findChild<QLabel*>("SearchResultMeta");
	if (metaLabel) {
		metaLabel->setStyleSheet(
			QString("QLabel{"
				"font-size: 12px;"
				"color: %1;"
				"}")
			.arg(DesignSystem::instance()->secondaryTextColor().name()));
	}
}

void HomePage::blockItemSignals(bool block)
{
	for (int i = 0; i < m_searchResultsList->count(); ++i) {
		QListWidgetItem* item = m_searchResultsList->item(i);
		QWidget* itemWidget = m_searchResultsList->itemWidget(item);
		if (itemWidget) {
			QCheckBox* checkbox = itemWidget->findChild<QCheckBox*>();
			if (checkbox) {
				checkbox->blockSignals(block);
			}
		}
	}
}

void HomePage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void HomePage::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	updateSearchResultsPosition();
}