#include "AntNumberInput.h"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include "DesignSystem.h"

AntNumberInput::AntNumberInput(QWidget* parent)
	: QWidget(parent), m_shadowColor(DesignSystem::instance()->primaryColor())
{
	setAttribute(Qt::WA_TranslucentBackground);  // 允许绘制阴影区域透明

	m_input = new AntBaseSpinBox(this);
	m_input->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 填充布局

	// 内边距为 6px，留出绘制阴影区域
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);  // 四周各 6px
	layout->addWidget(m_input);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_shadowColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

void AntNumberInput::paintEvent(QPaintEvent* event)
{
	if (!m_input->hasFocus())
		return;  // 什么都不做，直接退出

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// 参数定义
	int spread = 6;
	int baseAlpha = 80;
	int radius = 6;

	QRectF inputRect = m_input->geometry();  // 获取控件位置
	QColor baseColor = m_shadowColor;        // 设定的主题色或阴影色

	// 渐变圆角阴影绘制
	for (int i = 0; i < spread; ++i)
	{
		float ratio = 1.0f - float(i) / spread;
		int alpha = baseAlpha * ratio;

		QColor shadow = baseColor;
		shadow.setAlpha(alpha);

		QRectF shadowRect = inputRect.adjusted(-i, -i, i, i);
		QPainterPath path;
		path.addRoundedRect(shadowRect, radius + i, radius + i);

		painter.setPen(QPen(shadow, 1));
		painter.setBrush(Qt::NoBrush);
		painter.drawPath(path);
	}
}