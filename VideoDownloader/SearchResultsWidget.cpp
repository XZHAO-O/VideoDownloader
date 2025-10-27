#include "SearchResultsWidget.h"

#include <QLayout>
#include <QCheckBox>
#include <QLabel>
#include <QApplication>

#include "DesignSystem.h"
#include "StyleSheet.h"
#include "AntButton.h"
#include "AntChatListView.h"

// 完全自定义的委托类，不依赖基类
class SearchResultItemDelegate : public QStyledItemDelegate
{
public:
	explicit SearchResultItemDelegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent)
	{
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		Q_UNUSED(option);
		Q_UNUSED(index);
		return QSize(0, 60); // 固定高度为60像素
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		painter->save();
		painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

		QRect rect = option.rect;

		// 获取数据
		bool isChecked = index.data(Qt::UserRole + 5).toBool();
		QString title = index.data(Qt::UserRole + 1).toString();
		QString metaInfo = index.data(Qt::UserRole + 2).toString();

		// 获取当前主题颜色
		const Theme& theme = DesignSystem::instance()->currentTheme();

		// 绘制背景 - 处理选中和悬浮状态
		if (option.state & QStyle::State_Selected) {
			painter->fillRect(rect, theme.widgetSelectedBgColor);
		}
		else if (option.state & QStyle::State_MouseOver) {
			painter->fillRect(rect, theme.widgetHoverBgColor);
		}
		else {
			painter->fillRect(rect, theme.cardBackgroundColor);
		}

		// 绘制复选框
		const int checkboxSize = 16;
		int checkboxLeft = rect.left() + 12;
		int checkboxTop = rect.top() + (rect.height() - checkboxSize) / 2;
		QRect checkboxRect(checkboxLeft, checkboxTop, checkboxSize, checkboxSize);

		// 绘制复选框边框和背景
		QColor borderColor = theme.borderColor;
		QColor backgroundColor = theme.cardBackgroundColor;
		QColor primaryColor = theme.primaryColor;

		painter->setPen(QPen(borderColor, 1));
		painter->setBrush(isChecked ? primaryColor : backgroundColor);
		painter->drawRect(checkboxRect);

		// 如果选中，绘制勾选标记
		if (isChecked) {
			painter->setPen(QPen(theme.textColor, 2));
			painter->drawLine(checkboxRect.left() + 3, checkboxRect.center().y(),
				checkboxRect.center().x() - 1, checkboxRect.bottom() - 3);
			painter->drawLine(checkboxRect.center().x() - 1, checkboxRect.bottom() - 3,
				checkboxRect.right() - 3, checkboxRect.top() + 3);
		}

		// 绘制文本信息
		int textLeft = checkboxRect.right() + 12;
		int textWidth = rect.right() - textLeft - 12;
		QRect textRect(textLeft, rect.top(), textWidth, rect.height());

		// 标题
		QFont titleFont = QApplication::font();
		titleFont.setPointSizeF(titleFont.pointSizeF() - 1);
		QFontMetrics fmTitle(titleFont);

		QRect titleRect(textRect.left(), textRect.top() + 12, textWidth, fmTitle.height());
		painter->setFont(titleFont);
		painter->setPen(theme.primaryTextColor);

		QString elidedTitle = fmTitle.elidedText(title, Qt::ElideRight, textWidth);
		painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

		// 元信息（时长和作者）
		QFont metaFont = QApplication::font();
		metaFont.setPointSizeF(metaFont.pointSizeF() - 2);
		QFontMetrics fmMeta(metaFont);

		QRect metaRect(textRect.left(), titleRect.bottom() + 4, textWidth, fmMeta.height());
		painter->setFont(metaFont);
		painter->setPen(theme.secondaryTextColor);

		painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter, metaInfo);

		painter->restore();
	}

	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override
	{
		if (event->type() == QEvent::MouseButtonRelease) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			QRect rect = option.rect;

			// 计算复选框区域
			const int checkboxSize = 16;
			int checkboxLeft = rect.left() + 12;
			int checkboxTop = rect.top() + (rect.height() - checkboxSize) / 2;
			QRect checkboxRect(checkboxLeft, checkboxTop, checkboxSize, checkboxSize);

			// 只有在复选框区域内点击才改变状态
			if (checkboxRect.contains(mouseEvent->pos())) {
				bool currentState = index.data(Qt::UserRole + 5).toBool();
				model->setData(index, !currentState, Qt::UserRole + 5);
				return true; // 事件已处理
			}
		}
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}
};

SearchResultsWidget::SearchResultsWidget(QWidget* parent)
	: QWidget(parent)
	, m_totalItems(0)
{
	setObjectName("SearchResultsWidget");
	setupUI();
	setupConnections();
	updateStyle();
}

SearchResultsWidget::~SearchResultsWidget()
{
	// 移除事件过滤器
	if (m_selectAllCheckBox)
		m_selectAllCheckBox->removeEventFilter(this);
}

void SearchResultsWidget::setupUI()
{
	setObjectName("SearchResultsContainer");

	QVBoxLayout* containerLayout = new QVBoxLayout(this);
	containerLayout->setContentsMargins(16, 16, 16, 16);
	containerLayout->setSpacing(12);

	// 已选择数量标签
	m_selectedCountLabel = new QLabel("已选择 0/0", this);

	// 创建列表视图 - 完全模仿DownloadPage中的方式
	m_searchResultsList = new AntChatListView(this);
	m_searchResultsList->setObjectName("SearchResultsList");

	// 设置列表视图的属性，确保与DownloadPage一致
	m_searchResultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_searchResultsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	// 创建模型
	m_listModel = new QStandardItemModel(this);
	m_searchResultsList->setModel(m_listModel);

	// 设置完全自定义的委托
	SearchResultItemDelegate* delegate = new SearchResultItemDelegate(this);
	m_searchResultsList->setItemDelegate(delegate);

	// 底部操作栏
	QHBoxLayout* bottomLayout = new QHBoxLayout();
	bottomLayout->setContentsMargins(0, 0, 0, 0);
	bottomLayout->setSpacing(12);

	// 全选复选框 - 设置为三态
	m_selectAllCheckBox = new QCheckBox("全选", this);
	m_selectAllCheckBox->setTristate(true);
	m_selectAllCheckBox->installEventFilter(this);

	// 下一步按钮
	m_nextButton = new AntButton("下一步", 12, this);
	m_nextButton->setFixedSize(100, 36);

	bottomLayout->addWidget(m_selectAllCheckBox);
	bottomLayout->addStretch();
	bottomLayout->addWidget(m_nextButton);

	// 组装容器布局
	containerLayout->addWidget(m_selectedCountLabel);
	containerLayout->addWidget(m_searchResultsList, 1);
	containerLayout->addLayout(bottomLayout);
}

void SearchResultsWidget::setupConnections()
{
	connect(m_selectAllCheckBox, &QCheckBox::checkStateChanged,
		this, &SearchResultsWidget::onSelectAllStateChanged);
	connect(m_nextButton, &AntButton::clicked,
		this, &SearchResultsWidget::onNextButtonClicked);

	// 连接主题切换信号
	connect(DesignSystem::instance(), &DesignSystem::themeChanged,
		this, &SearchResultsWidget::onThemeChanged);

	// 连接模型数据变化信号
	connect(m_listModel, &QStandardItemModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
		for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
			QModelIndex index = m_listModel->index(row, 0);
			int itemIndex = row;
			bool isChecked = index.data(Qt::UserRole + 5).toBool();

			if (isChecked) {
				if (!m_selectedIndexes.contains(itemIndex)) {
					m_selectedIndexes.append(itemIndex);
				}
			}
			else {
				m_selectedIndexes.removeAll(itemIndex);
			}
		}

		updateSelectedCount();
		updateSelectAllCheckboxState();
		emit selectionChanged();
		});
}

void SearchResultsWidget::onThemeChanged()
{
	updateStyle();
	// 强制刷新列表视图
	m_searchResultsList->update();
}

void SearchResultsWidget::addSearchResultItem(const QString& title, const QString& duration, const QString& author)
{
	QStandardItem* item = new QStandardItem();

	// 存储数据到自定义角色
	item->setData(title, Qt::UserRole + 1);                    // 标题
	item->setData(QString("%1 · %2").arg(duration).arg(author), Qt::UserRole + 2); // 元信息
	item->setData(false, Qt::UserRole + 5);                   // 复选框状态

	m_listModel->appendRow(item);
	m_totalItems++;

	updateSelectedCount();
	updateSelectAllCheckboxState();
}

void SearchResultsWidget::addSearchResultItems(const QList<VideoInfo>& videoInfos)
{
	// 批量添加搜索结果，提高性能
	// 临时禁用视图更新以优化性能
	m_searchResultsList->setUpdatesEnabled(false);

	for (const VideoInfo& video : videoInfos) {
		if (video.isValid()) {
			QStandardItem* item = new QStandardItem();
			item->setData(video.title, Qt::UserRole + 1);
			item->setData(QString("%1 · %2").arg(video.duration).arg(video.author), Qt::UserRole + 2);
			item->setData(false, Qt::UserRole + 5);
			m_listModel->appendRow(item);
			m_totalItems++;
		}
	}

	// 重新启用视图更新
	m_searchResultsList->setUpdatesEnabled(true);

	// 只更新一次UI状态
	updateSelectedCount();
	updateSelectAllCheckboxState();
}

void SearchResultsWidget::clearAll()
{
	m_listModel->clear();
	m_selectedIndexes.clear();
	m_totalItems = 0;
	m_selectAllCheckBox->setCheckState(Qt::Unchecked);
	updateSelectedCount();
}

int SearchResultsWidget::getTotalItems() const
{
	return m_totalItems;
}

int SearchResultsWidget::getSelectedCount() const
{
	return m_selectedIndexes.size();
}

void SearchResultsWidget::updateStyle()
{
	const Theme& theme = DesignSystem::instance()->currentTheme();

	// 更新容器样式
	setStyleSheet(
		QString("#SearchResultsContainer{"
			"background-color: %1;"
			"border: 1px solid %2;"
			"border-radius: 8px;"
			"}")
		.arg(theme.cardBackgroundColor.name())
		.arg(theme.borderColor.name()));

	// 更新已选择数量标签样式
	m_selectedCountLabel->setStyleSheet(
		QString("QLabel{"
			"font-size: 14px;"
			"color: %1;"
			"font-weight: bold;"
			"}")
		.arg(theme.primaryColor.name()));

	// 为列表视图设置样式 - 模仿DownloadPage中的设置
	QString listStyle = StyleSheet::vListViewQss(theme.popupScrollBarColor);
	m_searchResultsList->setStyleSheet(listStyle);

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
		.arg(theme.primaryTextColor.name())
		.arg(theme.borderColor.name())
		.arg(theme.cardBackgroundColor.name())
		.arg(theme.primaryColor.name()));

	// 更新按钮样式
	//m_nextButton->updateStyle();
}

void SearchResultsWidget::setVisible(bool visible)
{
	QWidget::setVisible(visible);
	if (!visible) {
		clearAll();
	}
}

void SearchResultsWidget::onSelectAllStateChanged(int state)
{
	Q_UNUSED(state);
	// 实际处理在handleSelectAllClick中
}

void SearchResultsWidget::onNextButtonClicked()
{
	if (m_selectedIndexes.isEmpty()) {
		return;
	}
	emit nextButtonClicked();
}

void SearchResultsWidget::handleSelectAllClick()
{
	Qt::CheckState currentState = m_selectAllCheckBox->checkState();
	Qt::CheckState newState;

	if (currentState == Qt::Checked) {
		newState = Qt::Unchecked;
	}
	else {
		newState = Qt::Checked;
	}

	m_selectAllCheckBox->blockSignals(true);
	m_selectAllCheckBox->setCheckState(newState);
	m_selectAllCheckBox->blockSignals(false);

	// 更新所有项的选中状态
	for (int i = 0; i < m_listModel->rowCount(); ++i) {
		QModelIndex index = m_listModel->index(i, 0);
		m_listModel->setData(index, (newState == Qt::Checked), Qt::UserRole + 5);
	}

	updateSelectedCount();
	emit selectionChanged();
}

void SearchResultsWidget::updateSelectedCount()
{
	int selectedCount = m_selectedIndexes.size();
	m_selectedCountLabel->setText(QString("已选择 %1/%2").arg(selectedCount).arg(m_totalItems));
	m_nextButton->setEnabled(selectedCount > 0);
}

void SearchResultsWidget::updateSelectAllCheckboxState()
{
	if (m_totalItems == 0) {
		m_selectAllCheckBox->setCheckState(Qt::Unchecked);
		return;
	}

	int selectedCount = m_selectedIndexes.size();
	m_selectAllCheckBox->blockSignals(true);

	if (selectedCount == 0) {
		m_selectAllCheckBox->setCheckState(Qt::Unchecked);
	}
	else if (selectedCount == m_totalItems) {
		m_selectAllCheckBox->setCheckState(Qt::Checked);
	}
	else {
		m_selectAllCheckBox->setCheckState(Qt::PartiallyChecked);
	}

	m_selectAllCheckBox->blockSignals(false);
}

bool SearchResultsWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_selectAllCheckBox && event->type() == QEvent::MouseButtonRelease) {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::LeftButton) {
			handleSelectAllClick();
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}