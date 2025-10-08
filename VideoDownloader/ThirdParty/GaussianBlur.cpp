#include "GaussianBlur.h"
#include "ThirdParty/FastGaussianBlur-main/fast_gaussian_blur_template.h"
#include <cstring>

GaussianBlur::GaussianBlur(float sigma, int passes, BorderStrategy border)
	: m_sigma(sigma), m_passes(passes), m_border(border)
{
}

QImage GaussianBlur::blur(const QImage& src)
{
	if (src.isNull())
		return QImage();

	// 1. 转格式（RGBA8888 或 RGB32）
	QImage img = src.convertToFormat(QImage::Format_RGBA8888);

	int width = img.width();
	int height = img.height();
	int channels = 4;
	int size = width * height * channels;

	unsigned char* old_data = new unsigned char[size];
	unsigned char* new_data = new unsigned char[size];

	std::memcpy(old_data, img.bits(), size);

	// 2. 转换枚举
	Border border = Border::kMirror;
	switch (m_border)
	{
	case BorderStrategy::Extend:     border = Border::kExtend; break;
	case BorderStrategy::Mirror:     border = Border::kMirror; break;
	case BorderStrategy::KernelCrop: border = Border::kKernelCrop; break;
	case BorderStrategy::Wrap:       border = Border::kWrap; break;
	}

	// 3. 模糊
	fast_gaussian_blur(old_data, new_data, width, height, channels, m_sigma, m_passes, border);

	std::memcpy(img.bits(), new_data, size);

	delete[] old_data;
	delete[] new_data;

	return img;
}

void GaussianBlur::setSigma(float sigma)
{
	m_sigma = sigma;
}

void GaussianBlur::setPasses(int passes)
{
	m_passes = passes;
}

void GaussianBlur::setBorderStrategy(BorderStrategy border)
{
	m_border = border;
}