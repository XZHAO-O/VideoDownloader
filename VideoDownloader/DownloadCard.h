#pragma once

#include <QWidget>

#include "DownloadCardModel.h"
#include "VideoPreviewWindow.h"

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class AntButton;
class AntComboBox;
class AntCellWidget;
class MaterialProgressBar;
class SingleLevelComboBox;

class DownloadCard : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCard(QSharedPointer<DownloadCardModel> model, QWidget* parent = nullptr);
	~DownloadCard();

	QSharedPointer<DownloadCardModel> model() const { return m_model; }
	void setModel(QSharedPointer<DownloadCardModel> model);

	// 尺寸控制
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	void updateUI();

signals:
	void downloadClicked();
	void videoDownloadClicked();
	void audioDownloadClicked();
	void pauseClicked();
	void resumeClicked();
	void cancelClicked();
	void deleteClicked();
	void openFolderClicked();
	void copyUrlClicked();
	void openUrlClicked();
	void previewClicked();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private slots:
	void onModelChanged();
	void onCoverClicked();
	void onTitleClicked();
	void onVideoQualityChanged(const QString& quality);
	void onAudioQualityChanged(const QString& quality);
	void updateButtonStates();

private:
	void initUI();
	void initConnections();
	void initModelConnections();
	void updateVisibility();
	void updatePendingUI();
	void updateDownloadingUI();
	void updateDownloadedUI();
	void updateTextColors(); // 添加文本颜色更新函数

	// UI组件
	QLabel* m_coverLabel = nullptr;
	QLabel* m_playIcon = nullptr;
	AntCellWidget* m_titleCell = nullptr;  // 改为 AntCellWidget
	QLabel* m_sizeLabel = nullptr;
	QLabel* m_timeLabel = nullptr;
	QLabel* m_publisherLabel = nullptr;
	QLabel* m_progressLabel = nullptr;
	QLabel* m_speedLabel = nullptr;
	QLabel* m_progressInfoLabel = nullptr; // 新增：进度信息标签（已下载/总共）

	// 待下载状态按钮
	AntButton* m_downloadBtn = nullptr;
	AntButton* m_videoDownloadBtn = nullptr;
	AntButton* m_audioDownloadBtn = nullptr;
	AntButton* m_closeBtn = nullptr;

	// 下载中状态按钮（独立实例）
	AntButton* m_pauseBtn_downloading = nullptr;  // 下载中状态的暂停按钮
	AntButton* m_openFolderBtn_downloading = nullptr; // 下载中状态的打开文件夹按钮
	AntButton* m_deleteBtn_downloading = nullptr; // 下载中状态的删除按钮

	// 已下载状态按钮（独立实例）
	AntButton* m_openUrlBtn_downloaded = nullptr; // 已下载状态的打开链接按钮
	AntButton* m_openFolderBtn_downloaded = nullptr; // 已下载状态的打开文件夹按钮
	AntButton* m_deleteBtn_downloaded = nullptr; // 已下载状态的删除按钮

	SingleLevelComboBox* m_videoQualityCombo = nullptr;
	SingleLevelComboBox* m_audioQualityCombo = nullptr;
	MaterialProgressBar* m_progressBar = nullptr;

	QWidget* m_coverContainer = nullptr;
	QHBoxLayout* m_mainLayout = nullptr;
	QVBoxLayout* m_contentLayout = nullptr;
	QHBoxLayout* m_headerLayout = nullptr;
	QHBoxLayout* m_middleLayout = nullptr;
	QVBoxLayout* m_bottomLayout = nullptr;
	QHBoxLayout* m_actionLayout = nullptr;

	QSharedPointer<DownloadCardModel> m_model;
	QSharedPointer<VideoPreviewWindow> m_previewWindow;

	bool m_hovered = false;
	bool m_isCoverLoaded = false;
};