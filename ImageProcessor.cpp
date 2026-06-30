#include "ImageProcessor.hpp"
#include "Logger.hpp"

#include <QBuffer>
#include <QImage>

ImageProcessor *ImageProcessor::get() {
  static ImageProcessor instance;
  return &instance;
}

ImageProcessor::ImageProcessor() {}

QByteArray ImageProcessor::compressImage(const QString path, int maxBytes) {
  QImage image(path);
  if (image.isNull()) {
    Logger::Tag("ImageProcessor")
        .eFmt("Failed to load image: %s", path.toStdString().c_str());
    return {};
  }

  const QSize originalSize = image.size();

  // 统一转换为 ARGB32 格式，确保 PNG 编码兼容性
  if (image.format() != QImage::Format_ARGB32 &&
      image.format() != QImage::Format_ARGB32_Premultiplied) {
    image = image.convertToFormat(QImage::Format_ARGB32);
  }

  // 阶梯降分辨率：1920 → 1600 → 1280 → 1024 → 800
  constexpr int dimens[] = {1920, 1600, 1280, 1024, 800};
  constexpr int maxCompression = 100; // PNG 最高压缩级别

  QByteArray outputData;
  QSize resultSize = originalSize;
  int usedDimension = 0;

  for (int dim : dimens) {
    QImage scaled = (image.width() > dim || image.height() > dim)
                        ? image.scaled(dim, dim, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation)
                        : image;

    QBuffer buffer(&outputData);
    if (!buffer.open(QIODevice::WriteOnly)) {
      Logger::Tag("ImageProcessor").e("Failed to open buffer for writing");
      return {};
    }

    scaled.save(&buffer, "PNG", maxCompression);
    buffer.close();

    resultSize = scaled.size();
    usedDimension = dim;

    if (outputData.size() <= maxBytes) {
      break;
    }

    outputData.clear();

    // 已经是最后一档了，不再降，接受当前结果
    if (dim == dimens[sizeof(dimens) / sizeof(dimens[0]) - 1]) {
      // 重新编码一次保留数据
      QBuffer lastBuffer(&outputData);
      lastBuffer.open(QIODevice::WriteOnly);
      scaled.save(&lastBuffer, "PNG", 100);
      lastBuffer.close();
      break;
    }
  }

  Logger::Tag("ImageProcessor")
      .dFmt("Compressed: %dx%d -> %dx%d, %d bytes (target=%d, maxDim=%d)",
            originalSize.width(), originalSize.height(), resultSize.width(),
            resultSize.height(), outputData.size(), maxBytes, usedDimension);

  return outputData;
}
