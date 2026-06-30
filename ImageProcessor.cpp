#include "ImageProcessor.hpp"
#include "Logger.hpp"

#include <QBuffer>
#include <QImage>

ImageProcessor *ImageProcessor::get() {
  static ImageProcessor instance;
  return &instance;
}

ImageProcessor::ImageProcessor() {}

QByteArray ImageProcessor::compressImage(const QString path, int quality) {
  QImage image(path);
  if (image.isNull()) {
    Logger::Tag("ImageProcessor")
        .eFmt("Failed to load image: %s", path.toStdString().c_str());
    return {};
  }

  const QSize originalSize = image.size();

  // 如果图片尺寸过大，等比缩放至最大 1920 像素
  constexpr int kMaxDimension = 1920;
  if (image.width() > kMaxDimension || image.height() > kMaxDimension) {
    image = image.scaled(kMaxDimension, kMaxDimension, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
  }

  // 统一转换为 ARGB32 格式，确保 PNG 编码兼容性
  if (image.format() != QImage::Format_ARGB32 &&
      image.format() != QImage::Format_ARGB32_Premultiplied) {
    image = image.convertToFormat(QImage::Format_ARGB32);
  }

  QByteArray outputData;
  QBuffer buffer(&outputData);
  if (!buffer.open(QIODevice::WriteOnly)) {
    Logger::Tag("ImageProcessor").e("Failed to open buffer for writing");
    return {};
  }

  // PNG quality: -1=默认, 0=无压缩(大), 100=最大压缩(小)
  if (!image.save(&buffer, "PNG", quality)) {
    Logger::Tag("ImageProcessor").e("Failed to save compressed image");
    return {};
  }
  buffer.close();

  Logger::Tag("ImageProcessor")
      .dFmt("Compressed: %dx%d -> %dx%d, %d bytes (quality=%d)",
            originalSize.width(), originalSize.height(), image.width(),
            image.height(), outputData.size(), quality);

  return outputData;
}
