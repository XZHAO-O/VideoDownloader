#include "HomePage.h"
#include <QLayout>
#include "DesignSystem.h"

HomePage::HomePage(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("HomePage");

	QStringList listItems = {};
	antInput = new AntInput(300, listItems);
	antInput->setFixedWidth(400);
	antInput->setFixedHeight(50);
	antInput->setPlaceholderText("搜索内容");

	// 使用网格布局实现居中
	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setSpacing(0);

	// 将输入框放在网格的中心位置
	gridLayout->addWidget(antInput, 1, 1);

	// 设置行列的伸缩因子，使输入框居中
	gridLayout->setRowStretch(0, 1);  // 顶部弹性空间
	gridLayout->setRowStretch(1, 0);  // 中间行（固定高度）
	gridLayout->setRowStretch(2, 2);  // 底部弹性空间
	gridLayout->setColumnStretch(0, 1); // 左侧弹性空间
	gridLayout->setColumnStretch(1, 0); // 中间列（固定宽度）
	gridLayout->setColumnStretch(2, 1); // 右侧弹性空间
}

HomePage::~HomePage()
{
}

void HomePage::showEvent(QShowEvent* event)
{
	// 因为隐藏标题栏，所以需要调整视频窗口大小
	//resize(DesignSystem::instance()->contentSize());
}