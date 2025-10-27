#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class AntButton;
class AntToggleButton;
class BubbleViewController;
class DialogViewController;
class CircularAvatar;
class ModCardModel;
class ConfigVideoPlatform;

class ModCardWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ModCardWidget(QSharedPointer <ConfigVideoPlatform> configVideoPlatform, QSharedPointer<ModCardModel> model, BubbleViewController* bubbleView, DialogViewController* dialogView, QWidget* parent = nullptr);
	~ModCardWidget();

	QSharedPointer<ModCardModel> model() const { return m_model; }
	void setModel(QSharedPointer<ModCardModel> model);

	// 尺寸控制
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	void addDialog(DialogViewController* dialog);

signals:
	void toggleClicked(bool enabled);
	void openFolderClicked();
	void updateClicked();
	void uninstallClicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private slots:
	void onModelChanged();
	void onToggleClicked(bool checked);

	void onAvatarClicked();

private:
	void initUI();
	void initConnections();
	void showCustomModDialog(QUrl qrCodeUrl);
	void updateUI();
	QString processRichText(const QString& text);
	void updateTextColors();

	// UI组件
	QLabel* m_iconLabel = nullptr;
	QLabel* m_nameLabel = nullptr;
	QLabel* m_authorLabel = nullptr;
	QLabel* m_versionLabel = nullptr;
	QLabel* m_sizeLabel = nullptr;
	QLabel* m_descriptionTitleLabel = nullptr;
	QLabel* m_descriptionLabel = nullptr;

	AntToggleButton* m_toggleButton = nullptr;
	AntButton* m_openFolderButton = nullptr;
	AntButton* m_updateButton = nullptr;
	AntButton* m_uninstallButton = nullptr;

	QVBoxLayout* m_mainLayout = nullptr; // 改为垂直布局

	CircularAvatar* m_avatarButton = nullptr;
	BubbleViewController* m_bubbleView = nullptr;
	DialogViewController* m_dialogView = nullptr;

	QSharedPointer<ConfigVideoPlatform> m_configVideoPlatform;
	QSharedPointer<ModCardModel> m_model;
};