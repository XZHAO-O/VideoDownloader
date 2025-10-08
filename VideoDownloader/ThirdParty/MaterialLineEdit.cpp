#include "MaterialLineEdit.h"
#include <QPainter>
#include <QStyleOption>
#include <QFontMetrics>
#include <QToolButton>

MaterialLineEdit::MaterialLineEdit(QWidget* parent)
	: QLineEdit(parent),
	m_labelProgress(0.0),
	m_underlineProgress(0.0),
	m_padding(6),
	m_underlineAnimation(new QPropertyAnimation(this, "underlineProgress", this))
{
	setFrame(false);
	setPlaceholderText("");

	setStyleSheet(StyleSheet::lineEditQss(m_padding));
	m_borderColor = DesignSystem::instance()->currentTheme().borderColor;

	connect(this, &QLineEdit::textChanged, this, &MaterialLineEdit::onTextChanged);

	m_underlineAnimation->setEasingCurve(QEasingCurve::InOutSine);

	if (!text().isEmpty())
	{
		m_labelProgress = 1.0;
		m_underlineProgress = 1.0;
	}

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_borderColor = DesignSystem::instance()->currentTheme().borderColor;
			if (m_textBtn)
				m_textBtn->setStyleSheet(StyleSheet::noBorderBtnQss(DesignSystem::instance()->primaryColor()));
		});
}

MaterialLineEdit::~MaterialLineEdit()
{
	if (m_underlineAnimation)
		delete m_underlineAnimation;
}

void MaterialLineEdit::setLabelText(const QString& text)
{
	m_labelText = text;
	setPlaceholderText(m_labelText);
}

QString MaterialLineEdit::labelText() const
{
	return m_labelText;
}

void MaterialLineEdit::setTextFontSize(qreal size)
{
	QFont font;
	font.setPointSizeF(size);
	setFont(font);
}

void MaterialLineEdit::setPasswordToggleEnabled(bool enabled)
{
	m_togglePasswordEnabled = enabled;
	// 右侧小眼睛
	m_togglePasswordButton = new QToolButton(this);
	if (enabled)
	{
		m_togglePasswordButton->show();
		setEchoMode(QLineEdit::Password);  // 默认启用密码模式
		m_togglePasswordButton->setCheckable(true);
		m_togglePasswordButton->setChecked(false);
		m_togglePasswordButton->setCursor(Qt::PointingHandCursor);
		m_togglePasswordButton->setIcon(QIcon(":/Imgs/eye-slash-filled.svg"));
		m_togglePasswordButton->setIconSize(QSize(40, 40));
		m_togglePasswordButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");

		connect(m_togglePasswordButton, &QToolButton::toggled, this, [this](bool checked)
			{
				m_passwordVisible = checked;
				setEchoMode(m_passwordVisible ? QLineEdit::Normal : QLineEdit::Password);
				m_togglePasswordButton->setIcon(QIcon(m_passwordVisible
					? ":/Imgs/eye-filled.svg"
					: ":/Imgs/eye-slash-filled.svg"));
			});
	}
	else {
		if (m_togglePasswordButton)
		{
			m_togglePasswordButton->hide();
		}
		setEchoMode(QLineEdit::Normal);
	}

	if (m_togglePasswordEnabled && m_togglePasswordButton)
	{
		int buttonWidth = m_togglePasswordButton->size().width();
		setTextMargins(0, 0, buttonWidth + 10, 0);
	}
	else
	{
		setTextMargins(0, 0, 0, 0);
	}
}

void MaterialLineEdit::setRightTextBtn(QString text)
{
	//右侧文本
	setCursor(Qt::PointingHandCursor);
	m_textBtn = new QPushButton(this);
	m_textBtn->setStyleSheet(StyleSheet::noBorderBtnQss(DesignSystem::instance()->primaryColor()));
	m_textBtn->setText(text);
	m_textBtn->setMinimumSize(40, 40);
	m_textBtn->show();
}

qreal MaterialLineEdit::underlineProgress() const
{
	return m_underlineProgress;
}

void MaterialLineEdit::setUnderlineProgress(qreal progress)
{
	if (qFuzzyCompare(m_underlineProgress, progress))
		return;
	m_underlineProgress = progress;
	update();
}

void MaterialLineEdit::focusInEvent(QFocusEvent* event)
{
	m_themeColor = DesignSystem::instance()->primaryColor();
	m_underlineProgress = 0.0;
	QLineEdit::focusInEvent(event);
	animateUnderline(true);
}

void MaterialLineEdit::focusOutEvent(QFocusEvent* event)
{
	QLineEdit::focusOutEvent(event);
	animateUnderline(false);
}

void MaterialLineEdit::onTextChanged(const QString& /*newText*/)
{
	if (!text().isEmpty())
	{
		animateUnderline(true);
	}
	else
	{
		if (!hasFocus())
		{
			animateUnderline(false);
		}
	}
}

void MaterialLineEdit::animateUnderline(bool show)
{
	m_underlineAnimation->stop();

	qreal startValue = m_underlineProgress;  // 当前值

	if (show)
	{
		m_underlineAnimation->setStartValue(startValue);
		m_underlineAnimation->setEndValue(0.97);
		m_underlineAnimation->setDuration(300);
	}
	else
	{
		m_underlineAnimation->setStartValue(startValue);  // 改为当前值
		m_underlineAnimation->setEndValue(0.0);
		m_underlineAnimation->setDuration(300);
	}

	m_underlineAnimation->start();
}

void MaterialLineEdit::paintEvent(QPaintEvent* event)
{
	QLineEdit::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// --- 绘制圆角边框 ---
	QPen borderPen(m_borderColor);
	borderPen.setWidthF(1.5);  // 1.5px 宽度
	painter.setPen(borderPen);
	painter.setBrush(Qt::NoBrush);

	QRectF borderRect = rect().adjusted(2, 2, -2, -2);
	// 这里用6px圆角
	painter.drawRoundedRect(borderRect, 6.0, 6.0);

	// --- 绘制下划线动画 ---
	if (m_underlineProgress > 0.0)  // 始终绘制（包括失去焦点阶段）
	{
		// 焦点在时使用主题色，失去焦点时使用灰色
		QColor underlineColor = m_themeColor;
		QPen pen(underlineColor, 3);  // 控制横线粗细
		painter.setPen(pen);

		int lineY = height() - 2;
		int fullLength = width() - 2 * (m_padding - 2);  // 横线可覆盖整个控件宽度
		int centerX = width() / 2;
		int halfLength = int((fullLength / 2.0) * m_underlineProgress);  // 根据动画进度设置长度

		painter.drawLine(centerX - halfLength, lineY, centerX + halfLength, lineY);
	}
}

void MaterialLineEdit::resizeEvent(QResizeEvent* event)
{
	QLineEdit::resizeEvent(event);
	// 右侧小眼睛
	if (m_togglePasswordButton)
	{
		int buttonWidth = m_togglePasswordButton->size().width();
		setTextMargins(0, 0, buttonWidth + 10, 0);
		m_togglePasswordButton->move(width() - buttonWidth - 10, (height() - m_togglePasswordButton->height()) / 2 + 2);
	}
	// 右侧文本
	if (m_textBtn)
	{
		int textButtonWidth = m_textBtn->size().width();
		setTextMargins(0, 0, textButtonWidth + 10, 0);
		m_textBtn->move(width() - textButtonWidth - 10, (height() - m_textBtn->height()) / 2);
	}
}