#ifndef MATERIALLINEEDIT_H
#define MATERIALLINEEDIT_H

#include <QLineEdit>
#include <QPropertyAnimation>
#include <QColor>
#include <QPushButton>
#include "StyleSheet.h"
#include "DesignSystem.h"

class MaterialLineEdit : public QLineEdit
{
	Q_OBJECT
		Q_PROPERTY(qreal underlineProgress READ underlineProgress WRITE setUnderlineProgress)
public:
	explicit MaterialLineEdit(QWidget* parent = nullptr);
	~MaterialLineEdit() override;

	// 设置占位文本
	void setLabelText(const QString& text);
	QString labelText() const;

	// 设置字体大小
	void setTextFontSize(qreal size);

	// 控制下划线动画的进度
	qreal underlineProgress() const;
	void setUnderlineProgress(qreal progress);

	// 是否开启右侧的小眼睛(隐藏明文模式)
	void setPasswordToggleEnabled(bool enabled);

	// 设置右侧文本按钮启用
	void setRightTextBtn(QString text);

	// 当用户输入错误时候给予错误提示
	void errorHint()
	{
		m_themeColor = QColor(255, 77, 79);	// 经典红
		m_underlineProgress = 0.0;
		animateUnderline(true);
	}

	QString errorText() const
	{
		return m_errorText;
	}

	QPushButton* textBtn() { return m_textBtn; };

protected:
	void paintEvent(QPaintEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	void resizeEvent(QResizeEvent* event);
private slots:
	void onTextChanged(const QString& text);

private:
	void animateUnderline(bool toExpand);

	QString m_labelText;
	qreal m_labelProgress;
	qreal m_underlineProgress;

	QPropertyAnimation* m_labelAnimation;
	QPropertyAnimation* m_underlineAnimation;

	QColor m_themeColor = DesignSystem::instance()->primaryColor();	// 经典蓝
	QColor m_borderColor;
	int m_padding;

	QToolButton* m_togglePasswordButton = nullptr;
	QPushButton* m_textBtn = nullptr;
	bool m_passwordVisible = false;
	bool m_togglePasswordEnabled = false;

	QString m_errorText;            // 错误提示文本
};

#endif // MATERIALLINEEDIT_H
