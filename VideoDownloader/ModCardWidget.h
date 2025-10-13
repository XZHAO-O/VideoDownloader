// ModCardWidget.h
#pragma once

#include <QWidget>
#include <QSharedPointer>
#include "ModCardModel.h"

class QLabel;
class QPushButton;
class AntButton;
class AntToggleButton;
class QHBoxLayout;
class QVBoxLayout;

class ModCardWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ModCardWidget(QSharedPointer<ModCardModel> model, QWidget* parent = nullptr);
	~ModCardWidget();

	QSharedPointer<ModCardModel> model() const { return m_model; }
	void setModel(QSharedPointer<ModCardModel> model);

	// 尺寸控制
	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

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

private:
	void initUI();
	void initConnections();
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

	QSharedPointer<ModCardModel> m_model;
};