#pragma once

#include <QWidget>
#include <qevent.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qsvgrenderer.h>
#include <QElapsedTimer>
#include "BubbleViewController.h"
#include "DialogViewController.h"

class CircularAvatar : public QWidget
{
	Q_OBJECT

public:
	CircularAvatar(QSize size, QString prevImgPath, QString afterImgPath, QWidget* parent);
	~CircularAvatar();
	// 设置圆头像
	void setAvatar(QString svgFilePath);
	// 检查是否应该隐藏气泡框
	void checkShouldHideBubble();
	// 设置点击前点击后的头像
	void setImgs(QString prevImgPath, QString afterImgPath);
	// 添加对话框
	void addDialog(DialogViewController* dialog);
	// 允许登录
	void allowLogin(bool loginState);
	// 获得用户登录状态
	bool loginState() { return m_isLogin; };
protected:
	void paintEvent(QPaintEvent* e) override;
	void enterEvent(QEnterEvent* e);
	void leaveEvent(QEvent* e);
	void mousePressEvent(QMouseEvent* event) override;
signals:
	void playAnim(QPoint globalPos);
	void hideAnim();
	void showDialog(MaterialDialog::PageIndex index);
private:
	QSvgRenderer* m_prevSvg = nullptr;	//点击前头像
	QSvgRenderer* m_afterSvg = nullptr;	//点击后头像
	bool m_isClicked = false;
	// 气泡框
	BubbleViewController* m_bubble = nullptr;
	// 是否已进入过头像框
	bool m_isEnter = false;
	// 防抖设计
	QElapsedTimer m_lastEnterTime;
	const int m_enterIntervalMs = 300;
	// 自定义对话框
	DialogViewController* m_dialogView = nullptr;
	// 登录状态
	bool m_isLogin = false;
};
