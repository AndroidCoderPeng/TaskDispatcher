#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QByteArray>

class ImageProcessor {
public:
  static ImageProcessor *get();

  ImageProcessor(const ImageProcessor &) = delete;
  ImageProcessor &operator=(const ImageProcessor &) = delete;

  QByteArray compressImage(const QString path, int maxBytes = 2 * 1024 * 1024);

private:
  ImageProcessor();
};

#endif // IMAGEPROCESSOR_H
