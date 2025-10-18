#include "ModCardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QFileInfo>
#include <QTextDocument>
#include "AntButton.h"
#include "AntToggleButton.h"
#include "StyleSheet.h"
#include "DesignSystem.h"
#include "QrCodeWidget.h"

ModCardWidget::ModCardWidget(QSharedPointer < ConfigVideoPlatform> configVideoPlatform, QSharedPointer<ModCardModel> model, QWidget* parent)
	: QWidget(parent)
	, m_configVideoPlatform(configVideoPlatform)
	, m_model(model)
{
	setObjectName("ModCardWidget");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumHeight(400);
	setAttribute(Qt::WA_Hover, false);

	initUI();
	initConnections();

	if (m_model) {
		onModelChanged();
	}

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
		update();
		updateUI();
		});
}


ModCardWidget::~ModCardWidget()
{
}

void ModCardWidget::setModel(QSharedPointer<ModCardModel> model)
{
	if (m_model == model) return;

	if (m_model) {
		disconnect(m_model.get(), nullptr, this, nullptr);
	}

	m_model = model;

	if (m_model) {
		connect(m_model.get(), &ModCardModel::enabledChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::nameChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::authorChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::versionChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::sizeChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::descriptionChanged, this, &ModCardWidget::onModelChanged);
		connect(m_model.get(), &ModCardModel::iconPathChanged, this, &ModCardWidget::onModelChanged);

		onModelChanged();
	}
}

QSize ModCardWidget::sizeHint() const
{
	return QSize(600, 400); // 调整为更大的尺寸
}

QSize ModCardWidget::minimumSizeHint() const
{
	return QSize(400, 400);
}

void ModCardWidget::addDialog(DialogViewController* dialog)
{
	m_dialogView = dialog;
	connect(m_avatarButton, &CircularAvatar::showDialog, m_dialogView, &DialogViewController::showAnim);
}

void ModCardWidget::mousePressEvent(QMouseEvent* event)
{
	QWidget::mousePressEvent(event);
}

void ModCardWidget::enterEvent(QEnterEvent* event)
{
	// 移除悬浮效果
	QWidget::enterEvent(event);
}

void ModCardWidget::leaveEvent(QEvent* event)
{
	// 移除悬浮效果
	QWidget::leaveEvent(event);
}

void ModCardWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景 - 移除悬浮效果
	QRect bgRect = rect();
	QColor bgColor = DesignSystem::instance()->currentTheme().cardBackgroundColor;

	painter.setBrush(bgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(bgRect, 8, 8);

	QWidget::paintEvent(event);
}

void ModCardWidget::onModelChanged()
{
	if (!m_model) return;
	updateUI();
}

void ModCardWidget::onToggleClicked(bool checked)
{
	if (m_model) {
		m_model->setEnabled(checked);
		emit toggleClicked(checked);
	}
}

void ModCardWidget::initUI()
{
	// 主布局
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(24);
	m_mainLayout->setContentsMargins(24, 20, 24, 20);

	// 创建顶部布局，包含头像按钮
	QHBoxLayout* topLayout = new QHBoxLayout();
	topLayout->setContentsMargins(0, 0, 0, 0);
	topLayout->setSpacing(0);

	// 添加弹性空间，将头像推到右侧
	topLayout->addStretch();

	// 创建头像按钮
	m_avatarButton = new CircularAvatar(QSize(32, 32),
		":/Imgs/noLogin.svg",
		":/Imgs/github.svg",
		this);
	m_avatarButton->setToolTip("点击查看模组详情");
	topLayout->addWidget(m_avatarButton);

	// 第一部分：基本信息区域（图标 + 文字信息）
	QWidget* basicInfoWidget = new QWidget(this);
	QHBoxLayout* basicInfoLayout = new QHBoxLayout(basicInfoWidget);
	basicInfoLayout->setSpacing(20); // 增加图标和文字之间的间距
	basicInfoLayout->setContentsMargins(0, 0, 0, 0);

	// 左侧图标 - 使用专门的图标容器
	QWidget* iconContainer = new QWidget(this);
	iconContainer->setFixedSize(160, 160);
	QVBoxLayout* iconLayout = new QVBoxLayout(iconContainer);
	iconLayout->setContentsMargins(0, 0, 0, 0);
	iconLayout->setAlignment(Qt::AlignCenter);

	m_iconLabel = new QLabel(iconContainer);
	m_iconLabel->setFixedSize(150, 150); // 比容器稍小，确保完全显示
	m_iconLabel->setStyleSheet("QLabel{"
		"border-radius: 12px;" // 增大圆角
		"background-color: transparent;" // 透明背景
		"border: 2px solid #E0E0E0;" // 加粗边框
		"}");
	m_iconLabel->setAlignment(Qt::AlignCenter);
	m_iconLabel->setScaledContents(true); // 确保图片完全显示

	iconLayout->addWidget(m_iconLabel);

	// 右侧文字信息
	QWidget* textInfoWidget = new QWidget(this);
	QVBoxLayout* textInfoLayout = new QVBoxLayout(textInfoWidget);
	textInfoLayout->addStretch();
	textInfoLayout->setSpacing(12); // 增加行间距
	textInfoLayout->setContentsMargins(0, 0, 0, 0);

	// 模组名称 - 支持富文本
	m_nameLabel = new QLabel("模组名称", this);
	m_nameLabel->setTextFormat(Qt::RichText); // 启用富文本支持
	m_nameLabel->setOpenExternalLinks(false); // 禁用外部链接

	// 作者信息 - 支持富文本
	m_authorLabel = new QLabel("作者：未知", this);
	m_authorLabel->setTextFormat(Qt::RichText); // 启用富文本支持
	m_authorLabel->setOpenExternalLinks(false);

	// 版本信息
	m_versionLabel = new QLabel("版本：1.0.0", this);

	// 大小信息
	m_sizeLabel = new QLabel("大小：未知", this);

	textInfoLayout->addWidget(m_nameLabel);
	textInfoLayout->addWidget(m_authorLabel);
	textInfoLayout->addWidget(m_versionLabel);
	textInfoLayout->addWidget(m_sizeLabel);
	textInfoLayout->addStretch();

	// 组装基本信息区域
	basicInfoLayout->addWidget(iconContainer);
	basicInfoLayout->addWidget(textInfoWidget, 1);

	// 第二部分：介绍区域
	QWidget* descriptionWidget = new QWidget(this);
	QVBoxLayout* descriptionLayout = new QVBoxLayout(descriptionWidget);
	descriptionLayout->setSpacing(12); // 增加间距
	descriptionLayout->setContentsMargins(0, 0, 0, 0);

	m_descriptionTitleLabel = new QLabel("MOD介绍", this);
	m_descriptionLabel = new QLabel("模组描述信息将显示在这里...", this);
	m_descriptionLabel->setTextFormat(Qt::RichText); // 启用富文本支持
	m_descriptionLabel->setOpenExternalLinks(false);
	m_descriptionLabel->setWordWrap(true);
	m_descriptionLabel->setMaximumHeight(80); // 增加描述区域高度以容纳更多内容

	descriptionLayout->addWidget(m_descriptionTitleLabel);
	descriptionLayout->addWidget(m_descriptionLabel);

	// 第三部分：按钮区域
	QWidget* buttonWidget = new QWidget(this);
	QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
	buttonLayout->setSpacing(16); // 增加按钮间距
	buttonLayout->setContentsMargins(0, 0, 0, 0);

	// 开关按钮
	m_toggleButton = new AntToggleButton(QSize(60, 30), this); // 增大开关按钮
	m_toggleButton->setShowText(true);

	// 打开文件夹按钮
	m_openFolderButton = new AntButton("打开模组所在文件夹", 12, this); // 增大字体
	m_openFolderButton->setFixedSize(200, 40); // 增大按钮尺寸

	// 更新按钮
	m_updateButton = new AntButton("更新模组", 12, this);
	m_updateButton->setFixedSize(120, 40);

	// 卸载按钮
	m_uninstallButton = new AntButton("卸载模组", 12, this);
	m_uninstallButton->setFixedSize(120, 40);

	buttonLayout->addWidget(m_toggleButton);
	buttonLayout->addWidget(m_openFolderButton);
	buttonLayout->addWidget(m_updateButton);
	buttonLayout->addWidget(m_uninstallButton);
	buttonLayout->addStretch();

	// 组装主布局
	m_mainLayout->addLayout(topLayout);
	m_mainLayout->addWidget(basicInfoWidget);
	m_mainLayout->addWidget(descriptionWidget);
	m_mainLayout->addWidget(buttonWidget);

	// 初始更新文本颜色
	updateTextColors();
}

void ModCardWidget::initConnections()
{
	connect(m_toggleButton, &AntToggleButton::toggled, this, &ModCardWidget::onToggleClicked);
	connect(m_openFolderButton, &AntButton::clicked, this, &ModCardWidget::openFolderClicked);
	connect(m_updateButton, &AntButton::clicked, this, &ModCardWidget::updateClicked);
	connect(m_uninstallButton, &AntButton::clicked, this, &ModCardWidget::uninstallClicked);
	connect(m_avatarButton, &CircularAvatar::showDialog, this, &ModCardWidget::onAvatarClicked);
	connect(m_dialogView, &DialogViewController::successLogin, m_avatarButton, &CircularAvatar::allowLogin);
}

void ModCardWidget::onAvatarClicked()
{
	QUrl qrCodeUrl = m_configVideoPlatform->startQRCodeLogin();
	// 显示自定义的模组详情对话框
	showCustomModDialog(qrCodeUrl);
}

// 添加显示自定义模组对话框的函数
void ModCardWidget::showCustomModDialog(QUrl qrCodeUrl)
{
	if (!m_model) return;

	// 创建自定义内容
	QWidget* qrCodeLoginContent = new QWidget();
	qrCodeLoginContent->setFixedSize(250, 250);

	QHBoxLayout* contentLayout = new QHBoxLayout(qrCodeLoginContent);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(0);

	// 创建二维码
	QrCodeWidget* qrCode = new QrCodeWidget(qrCodeLoginContent);
	qrCode->setMinimumSize(QSize(150, 150));
	qrCode->setData(qrCodeUrl.toString());

	contentLayout->addStretch();
	contentLayout->addWidget(qrCode);
	contentLayout->addStretch();

	// 使用对话框控制器显示自定义内容
	m_dialogView->showQRCodeLoginDialog("模组详情", qrCodeLoginContent);
}

void ModCardWidget::updateUI()
{
	if (!m_model) return;

	// 更新文本颜色
	updateTextColors();

	// 更新基本信息 - 直接设置文本，QLabel会自动处理富文本
	m_nameLabel->setText(m_model->name());
	m_authorLabel->setText("作者：" + m_model->author());
	m_versionLabel->setText("版本：" + m_model->version());
	m_sizeLabel->setText("大小：" + m_model->formattedSize());
	m_descriptionLabel->setText(m_model->description());

	// 更新开关状态
	m_toggleButton->setChecked(m_model->enabled());

	// 加载图标
	qDebug() << "Loading icon for mod: " << m_model->iconPath();
	if (!m_model->iconPath().isEmpty() && QFile::exists(m_model->iconPath())) {
		QPixmap icon(m_model->iconPath());
		if (!icon.isNull()) {
			// 修复：使用图标标签的实际尺寸进行缩放
			QPixmap scaledIcon = icon.scaled(m_iconLabel->width(), m_iconLabel->height(),
				Qt::KeepAspectRatio, Qt::SmoothTransformation);
			m_iconLabel->setPixmap(scaledIcon);
		}
	}
	else {
		// 使用默认图标
		m_iconLabel->setText("MOD");
		// 设置默认图标的字体
		m_iconLabel->setStyleSheet(m_iconLabel->styleSheet() +
			QString("QLabel{"
				"font-size: 32px;" // 增大字体
				"font-weight: bold;"
				"color: #666666;"
				"}"));
	}
}

// 辅助函数：处理文本中的图片标签
QString ModCardWidget::processRichText(const QString& text)
{
	// 这里可以添加自定义的富文本处理逻辑
	// 例如：将特定的标记转换为HTML标签
	// 目前直接返回原文本，QLabel会自动处理基本的HTML标签
	return text;
}

void ModCardWidget::updateTextColors()
{
	auto theme = DesignSystem::instance()->currentTheme();

	// 更新名称颜色 - 增大字体
	m_nameLabel->setStyleSheet(QString("QLabel{"
		"font-size: 20px;" // 增大字体
		"font-weight: bold;"
		"color: %1;"
		"background-color: transparent;" // 透明背景
		"border: none;" // 无边框
		"}").arg(theme.primaryTextColor.name()));

	// 更新作者、版本、大小颜色 - 增大字体
	QString infoStyle = QString("QLabel{"
		"font-size: 16px;" // 增大字体
		"color: %1;"
		"background-color: transparent;" // 透明背景
		"border: none;" // 无边框
		"}").arg(theme.secondaryTextColor.name());

	m_authorLabel->setStyleSheet(infoStyle);
	m_versionLabel->setStyleSheet(infoStyle);
	m_sizeLabel->setStyleSheet(infoStyle);

	// 更新描述标题颜色 - 增大字体
	m_descriptionTitleLabel->setStyleSheet(QString("QLabel{"
		"font-size: 20px;" // 增大字体
		"font-weight: bold;"
		"color: %1;"
		"background-color: transparent;" // 透明背景
		"border: none;" // 无边框
		"}").arg(theme.primaryTextColor.name()));

	// 更新描述内容颜色 - 增大字体
	m_descriptionLabel->setStyleSheet(QString("QLabel{"
		"font-size: 16px;" // 增大字体
		"color: %1;"
		"background-color: transparent;" // 透明背景
		"border: none;" // 无边框
		"line-height: 1.4;" // 设置行高
		"}").arg(theme.secondaryTextColor.name()));

	// 更新图标边框颜色
	m_iconLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 12px;"
		"background-color: transparent;" // 透明背景
		"border: 2px solid %1;"
		"}").arg(theme.borderColor.name()));
}