#include "SingleLevelComboBox.h"
#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QStringListModel>
#include "DesignSystem.h"

SingleLevelComboBox::SingleLevelComboBox(QString showText, QStringList itemTextList, QWidget* parent, int popupHeight)
	: QWidget(parent),
	m_popupHeight(popupHeight),
	m_text(showText),
	m_itemTextList(itemTextList), // 存储选项列表
	m_borderColor(DesignSystem::instance()->currentTheme().popupBorderColor),
	m_shadowColor(DesignSystem::instance()->primaryColor()),
	m_isPressed(false),
	m_isChangeTextColor(false)
{
	m_popup = new PopupViewController(m_popupHeight, false, this);

	connect(m_popup, &PopupViewController::itemSelected, this, [this](const QModelIndex& idx)
		{
			QString selectedText = idx.data().toString();
			setCurrentText(selectedText);
			m_isChangeTextColor = false;
			m_popup->hideAnimated();
			DesignSystem::instance()->getTransparentMask()->hide();
		});

	connect(this, &SingleLevelComboBox::resized, this, [this](int width, int height)
		{
			m_popup->updateSize(width, height);
		});

	// 初始化下拉框模型
	updatePopupModel();

	connect(DesignSystem::instance()->getTransparentMask(), &TransparentMask::clickedOutside, this, [this]()
		{
			m_popup->raise();
			m_popup->hideAnimated();
			resetState();
			DesignSystem::instance()->getTransparentMask()->hide();
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_borderColor = DesignSystem::instance()->currentTheme().popupBorderColor;
			m_shadowColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

SingleLevelComboBox::~SingleLevelComboBox()
{
}

void SingleLevelComboBox::resetState()
{
	m_isPressed = false;
	m_isChangeTextColor = false;
	m_borderColor = DesignSystem::instance()->currentTheme().popupBorderColor;
	update();
}

void SingleLevelComboBox::setCurrentText(const QString& text)
{
	if (m_text != text) {
		m_text = text;
		update();
		emit currentTextChanged(text);
	}
}

void SingleLevelComboBox::setItemTextList(const QStringList& itemTextList)
{
	if (m_itemTextList != itemTextList) {
		m_itemTextList = itemTextList;
		updatePopupModel();
		emit itemTextListChanged(itemTextList);
	}
}

QStringList SingleLevelComboBox::itemTextList() const
{
	return m_itemTextList;
}

void SingleLevelComboBox::addItem(const QString& text)
{
	if (!m_itemTextList.contains(text)) {
		m_itemTextList.append(text);
		updatePopupModel();
		emit itemTextListChanged(m_itemTextList);
	}
}

void SingleLevelComboBox::removeItem(const QString& text)
{
	if (m_itemTextList.contains(text)) {
		m_itemTextList.removeAll(text);

		// 如果当前文本是被移除的项，则重置为第一个项或空字符串
		if (m_text == text) {
			if (!m_itemTextList.isEmpty()) {
				setCurrentText(m_itemTextList.first());
			}
			else {
				setCurrentText("");
			}
		}

		updatePopupModel();
		emit itemTextListChanged(m_itemTextList);
	}
}

void SingleLevelComboBox::clearItems()
{
	if (!m_itemTextList.isEmpty()) {
		m_itemTextList.clear();
		setCurrentText("");
		updatePopupModel();
		emit itemTextListChanged(m_itemTextList);
	}
}

void SingleLevelComboBox::updatePopupModel()
{
	// 创建新的模型
	QStandardItemModel* model = new QStandardItemModel(this);
	for (const QString& text : m_itemTextList)
	{
		QStandardItem* item = new QStandardItem(text);
		model->appendRow(item);
	}

	// 设置新模型
	if (m_popup && m_popup->popup) {
		m_popup->popup->setModel(model);
	}
}

void SingleLevelComboBox::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	int spread = 6;
	int baseAlpha = 80;
	int radius = 6;

	QRect rect = this->rect().adjusted(spread, spread, -spread, -spread);

	if (m_isPressed)
	{
		for (int i = 0; i < spread; ++i)
		{
			int alpha = baseAlpha * (1.0f - static_cast<float>(i) / spread);
			QColor shadow = m_shadowColor;
			shadow.setAlpha(alpha);
			QPen pen(shadow, 1.2);
			p.setPen(pen);
			p.setBrush(Qt::NoBrush);
			QRect shadowRect = rect.adjusted(-i, -i, i, i);
			p.drawRoundedRect(shadowRect, radius + i, radius + i);
		}
	}

	// 边框
	QPen pen(m_borderColor, 1.5);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(rect, radius, radius);

	// 文本
	if (m_isChangeTextColor)
	{
		p.setPen(Qt::gray);
	}
	else
	{
		p.setPen(DesignSystem::instance()->currentTheme().popupTextColor);
	}

	QFont font = p.font();
	font.setPointSize(11);
	p.setFont(font);
	QFontMetrics fm(font);

	// 文字居中显示 - 去掉右侧箭头占用的空间
	QRect textRect = rect;
	int textY = rect.top() + (rect.height() + fm.ascent() - fm.descent()) / 2;

	// 使用Qt的文本居中标志
	p.drawText(textRect, Qt::AlignCenter, m_text);
}

void SingleLevelComboBox::enterEvent(QEnterEvent*)
{
	if (m_isPressed) return;
	m_borderColor = DesignSystem::instance()->primaryColor();
	update();
}

void SingleLevelComboBox::leaveEvent(QEvent*)
{
	if (m_isPressed) return;
	m_borderColor = DesignSystem::instance()->borderColor();
	update();
}

void SingleLevelComboBox::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isPressed = true;
		m_isChangeTextColor = true;
		m_borderColor = DesignSystem::instance()->primaryColor();
		update();
		DesignSystem::instance()->getTransparentMask()->show();
		DesignSystem::instance()->getTransparentMask()->raise();
		m_popup->raise();
		QPoint popupPos = mapToGlobal(QPoint(0, height()));
		m_popup->showAnimated(popupPos, width());
	}
}

void SingleLevelComboBox::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	// 通知下拉弹窗更新尺寸
	emit resized(width(), height());
}