#pragma once

#include <QWidget>

#include "DownloadTaskInfo.h"
#include "VideoPreviewWindow.h"

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class AntButton;
class AntCellWidget;
class MaterialProgressBar;
class SingleLevelComboBox;
class SvgButton;
class SvgToggleButton;

enum class DownloadCardState
{
	Pending,        // 待下载
	Downloading,    // 下载中
	Downloaded,     // 已下载
	Error           // 错误
};

class DownloadCard : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCard(DownloadCardState downloadCardState, QWidget* parent = nullptr);
	~DownloadCard();

	// 完全更新卡片UI（传入完整任务信息）
	void updateFromTaskInfo(QSharedPointer<DownloadTaskInfo> taskInfo);

	// 部分更新方法（只更新指定部分）
	void updateProgress(const ProgressInfo& progress);
	void updateDownloadedFile(const QString& filePath);
	void updateQualityOptions(const QStringList& videoQualities, const QStringList& audioQualities);
	void updateSelectedQuality(const QString& videoQuality, const QString& audioQuality);
	void updateCover(const QByteArray& coverData);
	void updateTitle(const QString& title);
	void updateFileSizes(qint64 videoSize, qint64 audioSize);
	void updateTimeInfo(const QString& timeInfo);
	void updatePublisher(const QString& publisher);
	void updatePauseButton(DownloadStatus downloadStatus);

	// 设置质量选项
	void setVideoQualityOptions(const QStringList& qualities);
	void setAudioQualityOptions(const QStringList& qualities);

	// 设置当前选中的质量
	void setCurrentVideoQuality(const QString& quality);
	void setCurrentAudioQuality(const QString& quality);

	// 获取当前选中的质量
	QString currentVideoQuality() const;
	QString currentAudioQuality() const;

	// 尺寸控制
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

	// 获取当前UI状态
	DownloadCardState currentState() const { return m_currentState; }

	// 刷新UI（用于主题变化等）
	void refreshUI();

signals:
	void downloadClicked();
	void videoDownloadClicked();
	void audioDownloadClicked();
	void pauseClicked();
	void resumeClicked();
	void cancelClicked();
	void deleteClicked();
	void openFolderClicked();
	void openUrlClicked();
	void previewClicked();
	void retryClicked();

	void videoQualityChanged(const QString& quality);
	void audioQualityChanged(const QString& quality);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void onCoverClicked();
	void onTitleClicked();

private:
	// 初始化UI（根据状态）
	void initUI();

	// 根据状态初始化不同的UI
	void initPendingUI();
	void initDownloadingUI();
	void initDownloadedUI();
	void initErrorUI();

	// 初始化连接
	void initConnections();

	// 更新文本颜色（主题变化）
	void updateTextColors();

	// 设置播放图标
	void setupPlayIcon();
	void updatePlayIconVisibility(bool visible);

	// 清理UI组件
	void clearUI();

	// UI组件
	QLabel* m_coverLabel = nullptr;
	QLabel* m_playIcon = nullptr;
	AntCellWidget* m_titleCell = nullptr;
	QLabel* m_sizeLabel = nullptr;
	QLabel* m_timeLabel = nullptr;
	QLabel* m_publisherLabel = nullptr;

	// 布局
	QWidget* m_coverContainer = nullptr;
	QHBoxLayout* m_mainLayout = nullptr;
	QVBoxLayout* m_contentLayout = nullptr;
	QHBoxLayout* m_headerLayout = nullptr;
	QHBoxLayout* m_middleLayout = nullptr;
	QVBoxLayout* m_bottomLayout = nullptr;
	QHBoxLayout* m_actionLayout = nullptr;

	// 待下载状态特有组件
	SingleLevelComboBox* m_videoQualityCombo = nullptr;
	SingleLevelComboBox* m_audioQualityCombo = nullptr;
	AntButton* m_downloadBtn = nullptr;
	AntButton* m_videoDownloadBtn = nullptr;
	AntButton* m_audioDownloadBtn = nullptr;
	SvgButton* m_closeBtn = nullptr;

	// 下载中状态特有组件
	MaterialProgressBar* m_progressBar = nullptr;
	QLabel* m_speedLabel = nullptr;
	QLabel* m_progressInfoLabel = nullptr;
	SvgToggleButton* m_pauseBtn_downloading = nullptr;
	SvgButton* m_openFolderBtn_downloading = nullptr;
	SvgButton* m_deleteBtn_downloading = nullptr;

	// 已下载状态特有组件
	SvgButton* m_openUrlBtn_downloaded = nullptr;
	SvgButton* m_openFolderBtn_downloaded = nullptr;
	SvgButton* m_deleteBtn_downloaded = nullptr;

	// 错误状态特有组件
	QLabel* m_errorLabel = nullptr;
	SvgButton* m_retryBtn = nullptr;
	SvgButton* m_closeBtn_error = nullptr;

	QSharedPointer<VideoPreviewWindow> m_previewWindow;

	// UI状态信息
	DownloadCardState m_currentState;
	bool m_isCoverLoaded = false;
	bool m_coverHovered = false;
};