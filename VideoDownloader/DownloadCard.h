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
class SvgButton;

class DownloadCard : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCard(QSharedPointer<DownloadCardModel> model, QWidget* parent = nullptr);
	~DownloadCard();

	QSharedPointer<DownloadCardModel> model() const { return m_model; }
	void setModel(QSharedPointer<DownloadCardModel> model);

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

public slots:
	void onDownloadProgress(const QString& progressInfo, int progress);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

	// 新增事件过滤器
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void onModelChanged();
	void onCoverClicked();
	void onTitleClicked();
	void onVideoQualityChanged(const QString& quality);
	void onAudioQualityChanged(const QString& quality);

private:
	void initUI();
	void initConnections();
	void initModelConnections();
	void updateTextColors();

	// 新增封面图标相关方法
	void setupPlayIcon();
	void updatePlayIconVisibility(bool visible);

	// 根据状态初始化不同的UI
	void initPendingUI();
	void initDownloadingUI();
	void initDownloadedUI();

	// 根据状态更新不同的UI
	void updatePendingUI();
	void updateDownloadingUI();
	void updateDownloadedUI();

	// UI组件 - 公共部分
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
	SvgButton* m_pauseBtn_downloading = nullptr;
	SvgButton* m_openFolderBtn_downloading = nullptr;
	SvgButton* m_deleteBtn_downloading = nullptr;

	// 已下载状态特有组件
	SvgButton* m_openUrlBtn_downloaded = nullptr;
	SvgButton* m_openFolderBtn_downloaded = nullptr;
	SvgButton* m_deleteBtn_downloaded = nullptr;

	QSharedPointer<DownloadCardModel> m_model;
	QSharedPointer<VideoPreviewWindow> m_previewWindow;

	bool m_hovered = false;
	bool m_isCoverLoaded = false;
	bool m_coverHovered = false;  // 新增：标记封面是否被鼠标悬停
	DownloadCardState m_currentState;
};