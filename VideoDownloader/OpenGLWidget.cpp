#include"OpenGLWidget.h"

#include<QOpenGLTexture>
#include <QMessageBox>

constexpr float vertices[] =
{
	// 宽，高positions          // texture coords
	 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, // top right
	 1.0f, -1.0f, 0.0f,   1.0f, 1.0f, // bottom right
	-1.0f, -1.0f, 0.0f,   0.0f, 1.0f, // bottom left
	-1.0f,  1.0f, 0.0f,   0.0f, 0.0f  // top left 
};

constexpr unsigned int indices[] =
{
	0, 1, 3, 2  // 适用于TRIANGLE_STRIP的顺序
};

OpenGLWidget::OpenGLWidget(QWidget* parent)
	: QOpenGLWidget(parent)
{
	timer.setTimerType(Qt::PreciseTimer);
	connect(&timer, &QTimer::timeout, this, &OpenGLWidget::updateFrame);
}

OpenGLWidget::~OpenGLWidget()
{
	makeCurrent();
	if (texture)
		delete texture;
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	doneCurrent();
}

void OpenGLWidget::startTimer(double tpf)
{
	this->tpf = tpf;
	timer.start(tpf);
}

void OpenGLWidget::stopTimer()
{
	timer.stop();
}

void OpenGLWidget::updateFrameSize(int width, int height)
{
	frameWidth = width;
	frameHeight = height;
}

void OpenGLWidget::updateTexture()
{
	makeCurrent();
	if (texture->width() != frameWidth || texture->height() != frameHeight)
	{
		texture->destroy();
		texture->create();
		texture->setSize(frameWidth, frameHeight);
		texture->setFormat(QOpenGLTexture::RGB8_UNorm);
		texture->allocateStorage();
	}
	doneCurrent();
}

void OpenGLWidget::updateTransMatrix()
{
	// 根据控件宽高比计算缩放矩阵
	float widgetWidth = this->width();
	float widgetHeight = this->height();

	float scale_width = (float)widgetWidth / frameWidth;
	float scale_height = (float)widgetHeight / frameHeight;
	TransMatrix.setToIdentity();
	if (scale_width >= scale_height)
		TransMatrix.scale(frameWidth * scale_height / widgetWidth, 1);
	else
		TransMatrix.scale(1, frameHeight * scale_width / widgetHeight);
}

void OpenGLWidget::updateFrame()
{
	//if (!ctrl->muxerRunning || ctrl->playState != Control::PLAY)
		//return;
	lastUpdateTime = QTime::currentTime();
	//qDebug() << lastUpdateTime.toString("hh:mm:ss.zzz");
	//if (ctrl->frameQueue[ctrl->playIndex]->available)
	//{
	//	qDebug() << "emptywait";
	//	return;
	//}
	//frame = ctrl->frameQueue[ctrl->playIndex];
	Frame* frame = new Frame();
	makeCurrent();

	texture->bind();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, frame->data);

	doneCurrent();
	//ctrl->frameQueue[ctrl->playIndex]->available = true;
	//ctrl->playIndex = (ctrl->playIndex + 1) % ctrl->maxFrames;
	update();
}

void OpenGLWidget::initializeGL()
{
	initializeOpenGLFunctions();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/VertexShader.glsl");
	shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/FragShader.glsl");
	bool success = shader.link();
	if (!success)
		QMessageBox::critical(this, "Error", "着色器链接失败:" + shader.log());
	shader.bind();
	shader.setUniformValue("VertexColor", QVector3D(1.0f, 1.0f, 1.0f));
	// position attribute
	GLint pos = shader.attributeLocation("aPos");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	// texture coord attribute
	pos = shader.attributeLocation("aTexCoord");
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
	texture->create();
	texture->bind();
	shader.setUniformValue("texture0", 0);
	texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear); // 禁用Mipmap
	texture->setWrapMode(QOpenGLTexture::ClampToEdge); // 更适合视频

}

void OpenGLWidget::resizeGL(int w, int h)
{
	//Q_UNUSED(w);
	//Q_UNUSED(h);
	//glViewport(0, 0, w, h);
	qDebug() << "resizeGL:" << -QTime::currentTime().msecsTo(lastUpdateTime);
	if (-QTime::currentTime().msecsTo(lastUpdateTime) >= tpf)
	{
		updateFrame();
	}
	updateTransMatrix();
}

void OpenGLWidget::paintGL()
{
	shader.bind();

	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	shader.setUniformValue("TransMatrix", TransMatrix);
	texture->bind(0);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL);
}