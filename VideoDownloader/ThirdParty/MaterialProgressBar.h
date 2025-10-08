#ifndef MATERIALPROGRESSBAR_H
#define MATERIALPROGRESSBAR_H

#include <QProgressBar>
#include <QVariantAnimation>

class MaterialProgressBar : public QProgressBar
{
	Q_OBJECT
public:
	explicit MaterialProgressBar(QWidget* parent = nullptr);

	QSize sizeHint() const override;
	void setProgressAnimated(int targetValue);
	// 设置进度
	void setRealProgress(int targetValue);
	// 不确定动画
	void startIndeterminate()
	{
		if (m_indeterminateAnimation->state() != QAbstractAnimation::Running)
		{
			m_isBeginIndeterminate = true;
			m_indeterminateAnimation->start();
		}
	}

	void stopIndeterminate()
	{
		if (m_indeterminateAnimation->state() == QAbstractAnimation::Running)
		{
			m_isBeginIndeterminate = false;
			m_indeterminateAnimation->stop();
			m_indeterminatePosition = 0.0;
			update();
		}
	}
	// 开启演示模式 演示进度条动画流程 实际开发不要用
	void startTestPattern();
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	void updateProgress();
private:
	QColor m_backgroundColor;  // 背景颜色
	QColor m_chunkColor;       // 进度颜色
	QVariantAnimation* m_animation = nullptr;

	// 不确定动画
	bool m_isBeginIndeterminate = false;
	QVariantAnimation* m_indeterminateAnimation = nullptr;
	qreal m_indeterminatePosition = 0;  // 0.0 ~ 1.0 进度条宽度比例
	// 演示用值
	int currentValue = 0;
};

#endif // MATERIALPROGRESSBAR_H
