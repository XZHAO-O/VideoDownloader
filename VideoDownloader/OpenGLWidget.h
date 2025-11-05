#pragma once

#include<QOpenGLWidget>
#include<QOpenGLFunctions_3_3_Core>
#include<QOpenGLShaderProgram>
#include<QTimer>
#include<QTime>

class QOpenGLTexture;

class Frame
{
public:
	void* data;
	int width, height;
};

class OpenGLWidget :public QOpenGLWidget, QOpenGLFunctions_3_3_Core
{
	Q_OBJECT

public:
	explicit OpenGLWidget(QWidget* parent = nullptr);
	~OpenGLWidget();

	void startTimer(double tpf);
	void stopTimer();
	void updateFrameSize(int width, int height);
	void updateTexture();
	void updateTransMatrix();
	void updateFrame();

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

private:
	QOpenGLShaderProgram shader;
	QOpenGLTexture* texture;
	QMatrix4x4 TransMatrix;
	QTimer timer;
	QTime lastUpdateTime;
	QPoint lastPos;
	unsigned int VBO, VAO, EBO;
	double tpf = 0;
	int frameWidth, frameHeight;
};