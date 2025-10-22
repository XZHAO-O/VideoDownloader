#include "AntComboBox.h"
#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QStringListModel>
#include "DesignSystem.h"

AntComboBox::AntComboBox(QString showText, QStringList itemTextList, QWidget* parent, int popupHeight, bool enableMultiLevel,
	QMap<QString, QStringList> subItemMap)
	: QWidget(parent),
	m_text(showText),
	m_borderColor(DesignSystem::instance()->currentTheme().popupBorderColor),
	m_shadowColor(DesignSystem::instance()->primaryColor()),
	m_isPressed(false),
	m_isChangeTextColor(false),
	m_enableMultiLevel(enableMultiLevel)
{
	m_arrowRenderer = new QSvgRenderer(QStringLiteral(":/Imgs/downArrow.svg"), this);

	// 创建弹出框 - 二级列表足够了
	PopupViewController* popupView1 = new PopupViewController(popupHeight, enableMultiLevel, this);
	PopupViewController* popupView2 = new PopupViewController(popupHeight, false, this);
	m_popups.append(popupView1);
	m_popups.append(popupView2);

	// 获取弹出框引用以便在lambda中使用
	PopupViewController* popup1 = m_popups[0];
	PopupViewController* popup2 = m_popups[1];

	for (PopupViewController* popupView : m_popups)
	{
		connect(popupView, &PopupViewController::itemSelected, this, [this, popup1, popup2](const QModelIndex& idx)
			{
				PopupViewController* senderPopup = qobject_cast<PopupViewController*>(sender());

				if (senderPopup == popup1)
				{
					// 如果启用多级列表
					if (m_enableMultiLevel)
					{
						m_firstLevelSelectedText = idx.data().toString();  // 当前选中的一级文本

						if (m_subModels.contains(m_firstLevelSelectedText))
						{
							popup2->popup->setModel(m_subModels[m_firstLevelSelectedText]);
						}

						popup2->showAnimated(mapToGlobal(QPoint(width(), height())), width());
					}
					else
					{
						setCurrentText(idx.data().toString());
						m_isChangeTextColor = false;
						popup1->hideAnimated();
						DesignSystem::instance()->getTransparentMask()->hide();
					}
				}
				else if (senderPopup == popup2)
				{
					QString text = m_firstLevelSelectedText + " / " + idx.data().toString();
					setCurrentText(text);
					m_isChangeTextColor = false;
					popup1->hideAnimated();
					popup2->hideAnimated();
					resetState();
					DesignSystem::instance()->getTransparentMask()->hide();
				}
			});
	}

	connect(this, &AntComboBox::resized, this, [this](int width, int height)
		{
			for (PopupViewController* popup : m_popups)
			{
				popup->updateSize(width, height);
			}
		});

	// 添加数据模型
	QIcon arrowIcon(":/Imgs/rightArrow.svg");

	QStandardItemModel* model1 = new QStandardItemModel(this);
	for (const QString& text : itemTextList)
	{
		QStandardItem* item = new QStandardItem(text);
		// 设置箭头图标在右边：Qt 默认是左边显示图标，右边显示文本 坑爹
		// 这里需要用自定义代理实现右侧显示图标
		if (m_enableMultiLevel)
		{
			item->setData(arrowIcon, Qt::DecorationRole);
		}
		model1->appendRow(item);
	}
	popup1->popup->setModel(model1);

	// 如果启用多级列表，则设置二级列表
	if (m_enableMultiLevel)
	{
		for (const QString& key : subItemMap.keys())
		{
			QStandardItemModel* model = new QStandardItemModel(this);
			for (const QString& subText : subItemMap[key])
			{
				QStandardItem* item = new QStandardItem(subText);
				model->appendRow(item);
			}
			m_subModels[key] = model;
		}
	}

	connect(DesignSystem::instance()->getTransparentMask(), &TransparentMask::clickedOutside, this, [this, popup1, popup2]()
		{
			popup1->raise();
			popup2->raise();
			popup1->hideAnimated();
			if (m_enableMultiLevel) popup2->hideAnimated();
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

AntComboBox::~AntComboBox()
{
	if (m_arrowRenderer)
	{
		delete m_arrowRenderer;
		m_arrowRenderer = nullptr;
	}
	if (m_popups.size() > 0)
	{
		for (PopupViewController* popup : m_popups)
		{
			if (popup)
			{
				m_popups.removeOne(popup);

				delete popup;
				popup = nullptr;
			}
		}
	}
	if (m_subModels.size() > 0)
	{
		for (auto it = m_subModels.begin(); it != m_subModels.end(); ++it)
		{
			m_subModels.remove(it.key());
			if (it.value())
			{
				delete it.value();
				it.value() = nullptr;
			}
		}
	}
}

void AntComboBox::resetState()
{
	m_isPressed = false;
	m_isChangeTextColor = false;
	m_borderColor = DesignSystem::instance()->currentTheme().popupBorderColor;
	update();
}

void AntComboBox::setCurrentText(const QString& text)
{
	if (m_text != text) {
		m_text = text;
		update();
		emit currentTextChanged(text);
	}
}

void AntComboBox::setEnableMultiLevel(bool enable)
{
	m_enableMultiLevel = enable;
}

void AntComboBox::paintEvent(QPaintEvent*)
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
	int leftMargin = 12;
	int rightMargin = 28;
	QRect textRect = rect.adjusted(leftMargin, 0, -rightMargin, 0);
	int textY = rect.top() + (rect.height() + fm.ascent() - fm.descent()) / 2;
	p.drawText(textRect.left(), textY, m_text);

	// 箭头
	int arrowW = 20;
	int arrowH = 20;
	int arrowX = rect.right() - arrowW - 6;
	int arrowY = rect.top() + (rect.height() - arrowH) / 2;
	QRect arrowRect(arrowX, arrowY, arrowW, arrowH);
	m_arrowRenderer->render(&p, arrowRect);
}

void AntComboBox::enterEvent(QEnterEvent*)
{
	if (m_isPressed) return;
	m_borderColor = DesignSystem::instance()->primaryColor();
	update();
}

void AntComboBox::leaveEvent(QEvent*)
{
	if (m_isPressed) return;
	m_borderColor = DesignSystem::instance()->borderColor();
	update();
}

void AntComboBox::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isPressed = true;
		m_isChangeTextColor = true;
		m_borderColor = DesignSystem::instance()->primaryColor();
		update();
		DesignSystem::instance()->getTransparentMask()->show();
		DesignSystem::instance()->getTransparentMask()->raise();

		PopupViewController* popup1 = m_popups[0];
		PopupViewController* popup2 = m_popups[1];

		popup1->raise();
		if (m_enableMultiLevel) popup2->raise();
		QPoint popupPos = mapToGlobal(QPoint(0, height()));
		popup1->showAnimated(popupPos, width());
	}
}

void AntComboBox::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	// 通知下拉弹窗更新尺寸
	emit resized(width(), height());
}