#pragma once
#include <QString>
#include <QColor>

namespace StyleSheet
{

	inline QString mainQss(const QColor& bgColor)
	{
		return QString(R"(#main_widget {
					background-color: %1;
				})").arg(bgColor.name());
	}

	inline QString naviQss(const QColor& bgColor)
	{
		return QString(R"(#navi_widget{background-color: %1;
					})").arg(bgColor.name());
	}

	inline QString titleBarQss()
	{
		return QString(R"(#titleBar{background-color: transparent;
					})");
	}

	inline QString centralQss()
	{
		return QString(R"(#central{background-color: transparent;
					})");
	}

	inline QString gradientQss(const QColor& color1, const QColor& color2, const QColor& color3)
	{
		return QString(R"(
        #gradientWidget {
            background: qlineargradient(
                x1: 0, y1: 0,
                x2: 1, y2: 1,
                stop: 0 %1,
                stop: 0.4 %2,
                stop: 1 %3
            );
            border-radius: 12px;
        })")
			.arg(color1.name(), color2.name(), color3.name());
	}

	inline QString antMsgQss()
	{
		return R"(#AntMsg{background-color: gray;
					border-radius: 8px;})";
	}

	inline QString signInBtnQss()
	{
		return R"(
        #signInButton {
            background-color: #d0e9c6; /* 非常浅的薄荷绿 */
            border: none; /* 无边框 */
            border-radius: 6px; /* 现代圆角 */
            color: #388e3c; /* 深绿色文字 */
        }
        #signInButton:hover {
            background-color: #a5d6a7; /* 悬停时稍深的绿色 */
        }
        #signInButton:pressed {
            background-color: #81c784; /* 按压状态更深的绿色 */
        }
    )";
	}

	inline QString errorBtnQss()
	{
		return R"(/* 错误按钮 (红色系) */
					#errorButton {
						background-color: #E74C3C; /* 扁平红色 */
						border-color: #C0392B;
					}
					#errorButton:hover {
						background-color: #CB4335;
						border-color: #A93226;
					}
					#errorButton:pressed {
						background-color: #B03A2E;
					})";
	}

	inline QString infoBtnQss()
	{
		return R"(/* 信息按钮 (蓝色系 - 可以用基础样式，或略作调整) */
					#infoButton {
						background-color: #3498DB; /* 默认蓝色，也可以选一个特定信息蓝 */
						border-color: #2D709C;
					}
					#infoButton:hover {
						background-color: #2980B9;
						border-color: #2471A3;
					}
					#infoButton:pressed {
						background-color: #1F618D;
					})";
	}

	inline QString warningBtnQss()
	{
		return R"(/* 警告按钮 (橙色/黄色系) */
					#warningButton {
						background-color: #F39C12; /* 扁平橙色 */
						border-color: #D35400;
					}
					#warningButton:hover {
						background-color: #E67E22;
						border-color: #BA4A00;
					}
					#warningButton:pressed {
						background-color: #CA6F1E;
					})";
	}

	inline QString logoutBtnQss()
	{
		return R"(
        #logoutButton {
			color: rgb(80,80,80);
            background-color: #e2e6eb; /* 柔和的浅灰色 */
            border: none;
            border-radius: 6px;
        }
        #logoutButton:hover {
            background-color: #cbd5e0; /* 中等浅灰 */
        }
        #logoutButton:pressed {
            background-color: #a0aec0; /* 中灰色 */
        })";
	}

	inline QString myStackedQss(const QColor& bgColor)
	{
		return QString("#myStacked{background-color: rgb(%1, %2, %3); border-radius: 8px;}")
			.arg(bgColor.red())
			.arg(bgColor.green())
			.arg(bgColor.blue());
	}

	inline QString lineEditQss(int padding)
	{
		return QString(R"(
        QLineEdit {
			background-color: transparent;
            border: none;
			padding: %1;
        }
    )").arg(QString::number(padding));
	}

	inline QString noBorderBtnQss(const QColor& color)
	{
		return QString(R"(
        QPushButton {
            border: none;
			background-color: transparent;
            color: %1;
        })").arg(color.name());
	}

	inline QString titleBottomLineQss(const QColor& color)
	{
		return QString(R"(
        #dialogSubTitle {
            border: none;
            border-bottom: 2px solid %1; })").arg(color.name());
	}

	inline QString widgetLeftLineQss()
	{
		return QString(R"(
        #rightWidget {
            border: none;
            border-left: 2px solid rgb(200,200,200); })");
	}

	// 水平方向的标签Qss
	inline QString hTabQss(const QColor& themeColor, const QColor& textColor)
	{
		return QString(R"(
        QTabWidget::pane {
            border: none;
        }
        QTabBar {
            border: none;
            padding-left: 0px;
            padding-right: 0px;
        }
        QTabBar::tab {
            border: none;
			margin: 0px;	/* 禁用动画 */
            background: transparent;
            color: %1;  /* 普通标签文字颜色 */
        }
        QTabBar::tab:hover {
            color: %2;  /* hover 时文字颜色 */
        }
        QTabBar::tab:selected {
            color: %2;  /* 选中标签文字颜色 */
        })")
			.arg(textColor.name())
			.arg(themeColor.name());
	}

	inline QString toolBtnQss(const QColor& defaultColor = QColor("#BFBFBF"), const QColor& hoverColor = QColor("#1677FF"))
	{
		return QString(R"(
        QToolButton {
            color: %1;
            background-color: transparent;
            border: none;
        }
        QToolButton:hover {
            color: %2;
        } )").arg(defaultColor.name(), hoverColor.name());
	}

	inline QString standardDialogQss()
	{
		return "#standardDialog{ "
			"border-radius: 8px; "
			"background-color: white;}";
	}

	inline QString standardDialogBtnQss(const QColor& primaryColor, const QColor& bgColor)
	{
		return QString(R"(
        QPushButton#CancelButton {
            background-color: %2;
            color: %1;
            border-radius: 4px;
			border:1px solid %1;
        }

        QPushButton#CancelButton:hover {
            color: %1;
			background-color: %2;
			border:1px solid %1;
        }

        QPushButton#ConfirmButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 4px;
        }

        QPushButton#ConfirmButton:hover {
            background-color: %1;
        })").arg(primaryColor.name())
			.arg(bgColor.name());
	}

	inline QString notificationQss(const QColor& color)
	{
		return QString("#Notification{ "
			"border-radius: 8px; "
			"background-color: %1;}").arg(color.name());
	}

	inline QString AntBaseSpinBox(
		const QColor& normalBorder,
		const QColor& hoverBorder)
	{
		return QString(R"(
		#AntBaseSpinBox {
			border: 2px solid %1;
			border-radius: 6px;
		}

		#AntBaseSpinBox:hover,
		#AntBaseSpinBox:focus {
			border: 2px solid %2;
		}
		)")
			.arg(normalBorder.name())
			.arg(hoverBorder.name());
	}

	inline QString AntBaseDoubleSpinBoxQss(
		const QColor& normalBorder,
		const QColor& hoverBorder)
	{
		return QString(R"(
        #AntBaseDoubleSpinBox {
            border: 2px solid %1;
            border-radius: 6px;
        }

        #AntBaseDoubleSpinBox:hover,
		#AntBaseDoubleSpinBox:focus {
			border: 2px solid %2;
		}
		)")
			.arg(normalBorder.name())
			.arg(hoverBorder.name());
	}

	// 弹窗列表视图样式
	inline QString popupListViewQss(const QColor& handleColor)
	{
		return QString(R"(
		QListView {
			background-color: transparent;
			padding: 6px;
		}

		QScrollBar:vertical {
			width: 6px;
			background: transparent;
			border: none;
		}

		QScrollBar::handle:vertical {
			background: %1;
			border-radius: 3px;
		}

		QScrollBar::sub-line:vertical,
		QScrollBar::add-line:vertical {
			height: 0px;
			border: none;
			background: none;
		}

		QScrollBar::add-page:vertical,
		QScrollBar::sub-page:vertical {
			background: none;
		})")
			.arg(handleColor.name());
	}

	// 滚动区样式表
	inline QString antScrollAreaQss(const QColor& bgColor, const QColor& handleColor, const QColor& handleHoverColor)
	{
		return QString(R"(
		#AntScrollArea {
			background-color: %1;
			border: none;
		}

		#AntScrollArea QScrollBar:vertical, #AntScrollArea QScrollBar:horizontal {
			background: %1;
			border: none;
		}

		#AntScrollArea QScrollBar:vertical {
			width: 7px;
		}

		#AntScrollArea QScrollBar:horizontal {
			height: 7px;
		}

		#AntScrollArea QScrollBar::handle:vertical {
			background: %2;
			border-radius: 3px;
		}

		#AntScrollArea QScrollBar::handle:horizontal {
			background: %2;
			border-radius: 3px;
		}

		#AntScrollArea QScrollBar::handle:vertical:hover,
		#AntScrollArea QScrollBar::handle:horizontal:hover {
			background: %3;
			border-radius: 3px;
		}

		#AntScrollArea QScrollBar::handle:vertical:pressed,
		#AntScrollArea QScrollBar::handle:horizontal:pressed {
			background: %3;
		}

		#AntScrollArea QScrollBar::sub-line:vertical,
		#AntScrollArea QScrollBar::add-line:vertical,
		#AntScrollArea QScrollBar::sub-line:horizontal,
		#AntScrollArea QScrollBar::add-line:horizontal {
			width: 0px;
			height: 0px;
			border: none;
			background: none;
		}

		#AntScrollArea QScrollBar::add-page:vertical,
		#AntScrollArea QScrollBar::sub-page:vertical,
		#AntScrollArea QScrollBar::add-page:horizontal,
		#AntScrollArea QScrollBar::sub-page:horizontal {
			background: none;
		}
		)")
			.arg(bgColor.name())			// 设置背景颜色
			.arg(handleColor.name())		// 设置滚动条手柄颜色
			.arg(handleHoverColor.name());  // 设置悬停时的滚动条手柄颜色
	}

	inline QString antBaseInputQss(
		const QColor& normalBg,
		const QColor& hoverBg,
		const QColor& focusBorderColor,
		const QColor& focusBg,
		int padding)
	{
		return QString(R"(
		#AntBaseInput {
			border: 2px solid %1;
			border-radius: 6px;
			background-color: %1;
			padding: %5px;
		}

		#AntBaseInput:hover {
			background-color: %2;
		}

		#AntBaseInput:focus {
			border: 2px solid %3;
			background-color: %4;
		}
		)").arg(normalBg.name())
			.arg(hoverBg.name())
			.arg(focusBorderColor.name())
			.arg(focusBg.name())
			.arg(QString::number(padding));
	}

	// 垂直方向列表视图
	inline QString vListViewQss(const QColor& handleColor)
	{
		return QString(R"(
		QListView {
			background-color: transparent;
			border: none;
		}

		QScrollBar:vertical {
			width: 6px;
			background: transparent;
			border: none;
		}

		QScrollBar::handle:vertical {
			background: %1;
			border-radius: 3px;
		}

		QScrollBar::sub-line:vertical,
		QScrollBar::add-line:vertical {
			height: 0px;
			border: none;
			background: none;
		}

		QScrollBar::add-page:vertical,
		QScrollBar::sub-page:vertical {
			background: none;
		})")
			.arg(handleColor.name());
	}

	// align 文本的对齐方式
	inline QString textBtnQss(const QString& align, const QColor& color, const QColor& hoverColor)
	{
		return QString(R"(
        QPushButton {
			text-align: %1;
            border: none;
            background: transparent;
            color: %2;
        }
        QPushButton:hover {
            color: %3;
        }
    )").arg(align).arg(color.name()).arg(hoverColor.name());
	}

	inline QString antTabBarWidgetQss(const QColor& color)
	{
		return QString(R"(
		#AntTabBarWidget {
			background-color: %1;
		})").arg(color.name());
	}

	inline QString paginationWidgetQss(const QColor& color, const QColor& hoverColor)
	{
		return QString(R"(
        QPushButton {
            border: none;
        }
        QPushButton:hover {
            background-color: %1;
            border-radius: 6px;
        }
        QPushButton:checked {
            color: %2;
            border: 1px solid %2;
            border-radius: 6px;
            background: transparent;
        })")
			.arg(hoverColor.name())
			.arg(color.name());
	}
}
