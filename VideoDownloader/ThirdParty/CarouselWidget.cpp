#include "CarouselWidget.h"

CarouselWidget::CarouselWidget(QSize size, int showCardNums, QStringList imagePaths, QWidget* parent)
	: QWidget(parent),
	showCards(showCardNums)
{
	setFixedSize(size);

	// 限制 showCards 在合法范围内：最少 4 张，最多 7 张
	showCards = std::clamp(showCardNums, 4, 7);

	for (const QString& path : imagePaths)
	{
		CarouselCard* card = new CarouselCard(path, this);
		cards.append(card);
	}

	setupSizes();
	setupInitialLayout();

	// 创建定时器
	m_timer = new QTimer(this);
	m_timer->setInterval(2500);  // 2.5秒切换一次，可根据需要调整
	connect(m_timer, &QTimer::timeout, this, &CarouselWidget::showNext);
	m_timer->start();
}

void CarouselWidget::setupSizes()
{
	int W = width();
	int H = height();

	cardHeight = static_cast<int>(H * 0.95);
	expandedWidth = static_cast<int>(W * 0.55);

	// 限制最多显示7张卡片
	if (showCards > 7)
		showCards = 7;

	// 固定间距
	spacing = 20;

	int N = showCards;
	int K = N - 3;		// 中间收缩卡片数量
	if (K <= 0) K = 1;  // 最少有一个收缩卡片

	// 使用推导公式计算 shrinkWidth
	/*	W = left + expanded + shrink * K + spacing * (N - 1) + right
		W = (1 / 3 * S) + E + K * S + spacing * (N - 1) + (1 / 3 * S)
		W = (2 / 3 * S) + E + K * S + spacing * (N - 1)
		W = (K + 2 / 3) * S + E + spacing * (N - 1)
		(K + 2 / 3) * S = W - E - spacing * (N - 1)
		= > S = (W - E - spacing * (N - 1)) / (K + 2 / 3)*/
	double shrinkWidthF = (W - expandedWidth - spacing * (N - 1)) / (K + 2.0 / 3.0);
	shrinkWidth = static_cast<int>(shrinkWidthF);
}

void CarouselWidget::setupInitialLayout()
{
	int centerY = height() / 2;
	int invisibleLeft = shrinkWidth * 2 / 3;  // 左侧露出部分
	int visibleRight = shrinkWidth * 1 / 3;   // 右侧露出部分

	QSize expandedSize(expandedWidth, cardHeight);
	QSize shrinkSize(shrinkWidth, cardHeight);

	layoutTemplate.clear();

	// 首个是收缩卡片
	int a_x = rect().x() - invisibleLeft;
	cards[0]->resize(shrinkSize);
	cards[0]->move(a_x, centerY - cardHeight / 2);
	cards[0]->show();
	layoutTemplate.append(CardInfo{ QPoint(a_x, centerY - cardHeight / 2), shrinkSize });

	// 第二个是展开卡片
	int b_x = a_x + shrinkWidth + spacing;
	cards[1]->resize(expandedSize);
	cards[1]->move(b_x, centerY - cardHeight / 2);
	cards[1]->show();
	layoutTemplate.append(CardInfo{ QPoint(b_x, centerY - cardHeight / 2), expandedSize });

	// 设置中间的的卡片
	int shrink_x = b_x + expandedWidth + spacing;  // 从第二张卡片的位置开始
	for (int i = 2; i < showCards - 1; ++i)
	{
		// 计算当前卡片的位置：紧挨着前一张卡片
		cards[i]->resize(shrinkSize);
		cards[i]->move(shrink_x, centerY - cardHeight / 2);
		cards[i]->show();

		layoutTemplate.append(CardInfo{ QPoint(shrink_x, centerY - cardHeight / 2), shrinkSize });

		// 更新 shrink_x 为当前卡片的右侧
		shrink_x += shrinkWidth + spacing;
	}

	// 最后一个是收缩卡片
	int last_x = width() - visibleRight;
	cards[showCards - 1]->resize(shrinkSize);
	cards[showCards - 1]->move(last_x, centerY - cardHeight / 2);
	cards[showCards - 1]->show();
	layoutTemplate.append(CardInfo{ QPoint(last_x, centerY - cardHeight / 2), shrinkSize });

	// 其余隐藏
	for (int i = showCards; i < cards.size(); ++i)
		cards[i]->hide();
}

void CarouselWidget::showNext()
{
	if (animRunning) return;
	animRunning = true;
	carouselAnimation(currentStartIndex);
}

void CarouselWidget::carouselAnimation(int startIndex)
{
	if (cards.size() <= showCards) return;

	int centerY = height() / 2;
	QSize shrinkSize(shrinkWidth, cardHeight);

	// 动画组
	QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

	// 左侧隐藏位置
	int xOffset = layoutTemplate[1].pos.x() - layoutTemplate[0].pos.x();
	QPoint leftHidePos = QPoint(rect().x() - xOffset + layoutTemplate[0].pos.x(), centerY - cardHeight / 2);

	// 右侧隐藏位置
	QPoint rightHidePos = QPoint(layoutTemplate[showCards - 1].pos.x() + spacing + shrinkWidth, centerY - cardHeight / 2);

	// [0] 最左卡片滑出
	addCardAnimation(group,
		cards[0],
		QRect(layoutTemplate[0].pos, layoutTemplate[0].size),
		QRect(leftHidePos, shrinkSize));

	// [1 ~ showCards-1] 中间卡片左移
	for (int i = 1; i < showCards; ++i)
	{
		addCardAnimation(group,
			cards[i],
			QRect(layoutTemplate[i].pos, layoutTemplate[i].size),
			QRect(layoutTemplate[i - 1].pos, layoutTemplate[i - 1].size));
	}

	// [showCards] 新卡片滑入
	addCardAnimation(group,
		cards[showCards],
		QRect(rightHidePos, shrinkSize),
		QRect(layoutTemplate[showCards - 1].pos, layoutTemplate[showCards - 1].size));

	// 动画完成后轮转
	connect(group, &QParallelAnimationGroup::finished, this, [=]()
		{
			animRunning = false;
			CarouselCard* card = cards.takeFirst();
			cards.append(card);
		});

	group->start(QAbstractAnimation::DeleteWhenStopped);
}

void CarouselWidget::addCardAnimation(QParallelAnimationGroup* group, CarouselCard* card, const QRect& startRect, const QRect& endRect)
{
	card->show();
	auto* anim = new QPropertyAnimation(card, "geometry");
	anim->setDuration(500);
	anim->setEasingCurve(QEasingCurve::OutCubic);
	anim->setStartValue(startRect);
	anim->setEndValue(endRect);
	group->addAnimation(anim);
}