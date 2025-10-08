#pragma once

#include "AntBaseListView.h"

class AntChatListItemDelegate : public AntBaseListItemDelegate
{
public:
	explicit AntChatListItemDelegate(QObject* parent = nullptr)
		: AntBaseListItemDelegate(parent)
	{
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		const QString name = index.data(Qt::UserRole + 1).toString();
		const QString message = index.data(Qt::UserRole + 2).toString();

		// 设置用户名字体（加粗）
		QFont nameFont = option.font;
		nameFont.setBold(true);
		QFontMetrics nameMetrics(nameFont);
		int nameHeight = nameMetrics.height();

		// 普通字体用于消息
		QFont messageFont = option.font;
		QFontMetrics messageMetrics(messageFont);
		int messageHeight = messageMetrics.height();

		// 垂直方向 padding
		const int verticalPadding = 8;
		const int spacingBetweenLines = 4;

		// 总高度 = 顶部 padding + 名字高度 + 间距 + 消息高度 + 底部 padding
		int textBlockHeight = verticalPadding + nameHeight + spacingBetweenLines + messageHeight + verticalPadding;

		// 头像高度
		const int avatarHeight = 50;

		// 返回高度取文本区与头像中较高者
		int totalHeight = qMax(textBlockHeight, avatarHeight);

		return QSize(option.rect.width(), totalHeight);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		AntBaseListItemDelegate::paint(painter, option, index);

		painter->save();
		painter->setRenderHints(QPainter::Antialiasing | QPainter::LosslessImageRendering | QPainter::SmoothPixmapTransform);

		QRect rect = option.rect;

		// 1. 先定位头像位置，固定大小，顶对齐item顶端
		const int imageSize = 46;
		int imgMargin = (rect.height() - imageSize) / 2;
		QRect imageRect(rect.left() + imgMargin, rect.top() + imgMargin, imageSize, imageSize);

		// 实际头像尺寸，稍微缩小
		const int avatarSize = 42;
		QRect avatarRect(
			imageRect.left() + (imageRect.width() - avatarSize) / 2,
			imageRect.top() + (imageRect.height() - avatarSize) / 2,
			avatarSize, avatarSize
		);

		// 2. 绘制头像，圆角裁剪
		QString imagePath = index.data(Qt::DecorationRole).toString();
		QPixmap pixmap(imagePath);
		if (!pixmap.isNull())
		{
			int radius = 6; // 可根据 avatarSize 调整
			QPainterPath clipPath;
			clipPath.addRoundedRect(avatarRect, radius, radius);
			painter->setClipPath(clipPath);
			painter->drawPixmap(avatarRect, pixmap);
			painter->setClipping(false);
		}

		// 3. 根据头像位置绘制文本区域
		int textLeft = imageRect.right() + imgMargin;
		int textWidth = rect.right() - textLeft;
		QRect textRect(textLeft, rect.top(), textWidth, rect.height());

		QString userName = index.data(Qt::UserRole + 1).toString();
		QString message = index.data(Qt::UserRole + 2).toString();

		// 4. 字体和字体度量
		QFont userNameFont = option.font;
		userNameFont.setBold(true);
		userNameFont.setPointSizeF(userNameFont.pointSizeF() + 0.5);  // 放大0.5pt
		QFontMetrics fmUserName(userNameFont);
		int userNameHeight = fmUserName.height();

		QFont messageFont = option.font;
		messageFont.setPointSizeF(messageFont.pointSizeF() - 0.5);     // 缩小0.5pt
		QFontMetrics fmMessage(messageFont);
		int messageHeight = fmMessage.height();

		QColor original = DesignSystem::instance()->currentTheme().listTextColor;
		QColor lighter = DesignSystem::instance()->currentTheme().listItemDescColor;

		// 5. 用户名顶部对齐头像顶部
		QRect userNameRect(textRect.left(), imageRect.top(), textWidth, userNameHeight);
		painter->setFont(userNameFont);
		painter->setPen(original);
		painter->drawText(userNameRect, Qt::AlignLeft | Qt::AlignVCenter, userName);

		// 6. 消息底部对齐头像底部，单行省略
		QRect messageRect(textRect.left(), imageRect.bottom() - messageHeight + 1, textWidth, messageHeight);
		QString elidedMessage = fmMessage.elidedText(message, Qt::ElideRight, textWidth);
		painter->setFont(messageFont);
		painter->setPen(lighter);
		painter->drawText(messageRect, Qt::AlignLeft | Qt::AlignVCenter, elidedMessage);

		painter->restore();
	}

	void updateStyle()
	{
		updateBaseStyle();
	}
};

class AntChatListView : public AntBaseListView
{
	Q_OBJECT

public:
	// 示例数据
	struct ChatItem
	{
		QString avatarPath;  // 头像路径
		QString name;        // 昵称
		QString message;     // 消息内容
		QString time;        // 消息时间
		bool isSentByMe;     // 是否是自己发送的
	};

	AntChatListView(QWidget* parent);
	~AntChatListView();
	// 创建模型
	QStandardItemModel* createModel(const QVector<ChatItem>& chatItems);
private:
	AntChatListItemDelegate* itemDele;
};
