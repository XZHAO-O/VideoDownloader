#pragma once

#include <QMetaType>

// 应用程序状态枚举
enum ApplicationState {
	Uninitialized = 0,
	Initializing,
	Running,
	ShuttingDown,
	Error
};

Q_DECLARE_METATYPE(ApplicationState)