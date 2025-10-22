#pragma once

#include <QWidget>
#include <QSharedPointer>
#include "DownloadCardModel.h"
#include "VideoPreviewWindow.h"
#include "QNetworkAccessManager.h"
#include "AntCellWidget.h"

class QLabel;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class AntButton;
class AntComboBox;
class MaterialProgressBar;
class SingleLevelComboBox;

class DownloadCard : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCard(QSharedPointer<DownloadCardModel> model, QWidget* parent = nullptr);
	~DownloadCard();

	void cleanupMaterialProgressBar(MaterialProgressBar* progressBar);

	void cleanupAntButton(AntButton* button);

	QSharedPointer<DownloadCardModel> model() const { return m_model; }
	void setModel(QSharedPointer<DownloadCardModel> model);

	// 尺寸控制
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

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
	void updateUI();
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

	AntButton* m_downloadBtn = nullptr;
	AntButton* m_videoDownloadBtn = nullptr;
	AntButton* m_audioDownloadBtn = nullptr;
	AntButton* m_closeBtn = nullptr;
	AntButton* m_pauseBtn = nullptr;
	AntButton* m_deleteBtn = nullptr;
	AntButton* m_openFolderBtn = nullptr;
	AntButton* m_copyUrlBtn = nullptr;
	AntButton* m_openUrlBtn = nullptr;

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

	QNetworkAccessManager* m_networkManager = nullptr;
};