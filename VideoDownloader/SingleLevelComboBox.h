#pragma once
#include <QWidget>
#include <QColor>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QList>
#include <QMap>
#include <QStandardItemModel>
#include "DesignSystem.h"
#include "PopupViewController.h"

class SingleLevelComboBox : public QWidget
{
	Q_OBJECT
public:
	// popupHeight 弹出框高度
	explicit SingleLevelComboBox(QString showText, QStringList itemTextList, QWidget* parent = nullptr, int popupHeight = 160);

	~SingleLevelComboBox() override;

	void setCurrentText(const QString& text);
	QString currentText() const { return m_text; }

	// 新增：设置和获取选项列表
	void setItemTextList(const QStringList& itemTextList);
	QStringList itemTextList() const;

	// 新增：添加单个选项
	void addItem(const QString& text);
	// 新增：移除单个选项
	void removeItem(const QString& text);
	// 新增：清空所有选项
	void clearItems();

	TransparentMask* getMask() { return DesignSystem::instance()->getTransparentMask(); }
	PopupViewController* popupView() { return m_popup; };

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	// 恢复初始状态
	void resetState();
	// 更新下拉框模型
	void updatePopupModel();

signals:
	void resized(int width, int height);
	// 新增：当前文本改变信号
	void currentTextChanged(const QString& text);
	// 新增：选项列表改变信号
	void itemTextListChanged(const QStringList& itemTextList);

private:
	PopupViewController* m_popup = nullptr;
	int m_popupHeight;
	QString m_text;
	QColor m_borderColor;
	QColor m_shadowColor;

	// 新增：存储选项列表
	QStringList m_itemTextList;

	bool m_isPressed;
	bool m_isChangeTextColor;
};