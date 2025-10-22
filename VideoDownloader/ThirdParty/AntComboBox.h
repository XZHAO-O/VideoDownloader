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

class AntComboBox : public QWidget
{
	Q_OBJECT
public:
	// enableMultiLevel 是否启用多级列表 poupHeight 弹出框高度
	explicit AntComboBox(QString showText, QStringList itemTextList, QWidget* parent = nullptr, int popupHeight = 240, bool enableMultiLevel = false,
		QMap<QString, QStringList> subItemMap = QMap<QString, QStringList>());
	~AntComboBox();

	QString currentText() const { return m_text; }
	void setCurrentText(const QString& text);
	// 设置是否启用多级列表
	void setEnableMultiLevel(bool enable);
	TransparentMask* getMask() { return DesignSystem::instance()->getTransparentMask(); }
	QList<PopupViewController*>& popupViewList() { return m_popups; };

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	// 恢复初始状态
	void resetState();

signals:
	void resized(int width, int height);
	void currentTextChanged(const QString& text);

private:
	QList<PopupViewController*> m_popups; // 用于存储所有弹出框
	QString m_text;
	QString m_firstLevelSelectedText;
	QColor m_borderColor;
	QColor m_shadowColor;
	QSvgRenderer* m_arrowRenderer;
	QMap<QString, QStandardItemModel*> m_subModels;    // 全部二级列表模型

	bool m_isPressed;
	bool m_isChangeTextColor;
	bool m_enableMultiLevel = false;
};