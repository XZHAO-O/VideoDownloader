#pragma once

#include <QImage>

class GaussianBlur
{
public:
	enum class BorderStrategy
	{
		Extend,
		Mirror,
		KernelCrop,
		Wrap
	};

	GaussianBlur(float sigma, int passes, BorderStrategy border = BorderStrategy::Mirror);

	QImage blur(const QImage& src);

	void setSigma(float sigma);
	void setPasses(int passes);
	void setBorderStrategy(BorderStrategy border);

private:
	float m_sigma;
	int m_passes;
	BorderStrategy m_border;
};
