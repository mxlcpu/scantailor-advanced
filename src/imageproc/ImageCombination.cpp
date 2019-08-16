// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageCombination.h"
#include <QImage>
#include <unordered_map>
#include <unordered_set>
#include "BinaryImage.h"

namespace imageproc {
namespace impl {
template <typename MixedPixel>
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground) {
  auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* foreground_line = foreground.data();
  const int foreground_stride = foreground.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (foreground_line[x >> 5] & (msb >> (x & 31))) {
        uint32_t tmp = foreground_line[x >> 5];
        tmp >>= (31 - (x & 31));
        tmp &= uint32_t(1);

        --tmp;
        tmp |= 0xff000000;
        mixed_line[x] = static_cast<MixedPixel>(tmp);
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
  }
}

template <typename MixedPixel>
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* foreground_line = foreground.data();
  const int foreground_stride = foreground.wordsPerLine();
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask_line[x >> 5] & (msb >> (x & 31))) {
        uint32_t tmp = foreground_line[x >> 5];
        tmp >>= (31 - (x & 31));
        tmp &= uint32_t(1);

        --tmp;
        tmp |= 0xff000000;
        mixed_line[x] = static_cast<MixedPixel>(tmp);
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
    mask_line += mask_stride;
  }
}

template <typename MixedPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground) {
  auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const auto* foreground_line = reinterpret_cast<const MixedPixel*>(foreground.bits());
  const int foreground_stride = foreground.bytesPerLine() / sizeof(MixedPixel);
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((foreground_line[x] & msb) != msb) {
        mixed_line[x] = foreground_line[x];
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
  }
}

template <typename MixedPixel, typename ForegroundPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground);

template <>
void combineImagesColor<uint32_t, uint8_t>(QImage& mixedImage, const QImage& foreground) {
  auto* mixed_line = reinterpret_cast<uint32_t*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(uint32_t);
  const auto* foreground_line = foreground.bits();
  const int foreground_stride = foreground.bytesPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  const QVector<QRgb> foreground_palette = foreground.colorTable();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = foreground_palette[foreground_line[x]];
      if ((color & msb) != msb) {
        mixed_line[x] = color;
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
  }
}

void mergePalettes(QVector<uint32_t>& mixed_palette, const QVector<uint32_t>& palette) {
  std::unordered_set<uint32_t> mixed_colors(mixed_palette.begin(), mixed_palette.end());
  for (uint32_t color : palette) {
    if (mixed_colors.find(color) == mixed_colors.end()) {
      mixed_palette.push_back(color);
      mixed_colors.insert(color);
    }
  }
}

template <>
void combineImagesColor<uint8_t, uint8_t>(QImage& mixedImage, const QImage& foreground) {
  auto* mixed_line = mixedImage.bits();
  const int mixed_stride = mixedImage.bytesPerLine();
  const auto* foreground_line = foreground.bits();
  const int foreground_stride = foreground.bytesPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  QVector<uint32_t> mixed_palette = mixedImage.colorTable();
  const QVector<uint32_t> foreground_palette = foreground.colorTable();
  if (mixed_palette.size() < 256) {
    mergePalettes(mixed_palette, foreground_palette);
    if (mixed_palette.size() > 256) {
      mixed_palette.resize(256);
    }
    mixedImage.setColorTable(mixed_palette);
  }

  std::unordered_map<uint32_t, uint8_t> colorToIndex;
  for (int i = 0; i < mixed_palette.size(); ++i) {
    colorToIndex[mixed_palette[i]] = static_cast<uint8_t>(i);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = foreground_palette[foreground_line[x]];
      if ((color & msb) != msb) {
        mixed_line[x] = colorToIndex[color];
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
  }
}

template <typename MixedPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const auto* foreground_line = reinterpret_cast<const MixedPixel*>(foreground.bits());
  const int foreground_stride = foreground.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask_line[x >> 5] & (msb >> (x & 31))) {
        mixed_line[x] = foreground_line[x];
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
    mask_line += mask_stride;
  }
}

template <typename MixedPixel, typename ForegroundPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

template <>
void combineImagesColor<uint32_t, uint8_t>(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixed_line = reinterpret_cast<uint32_t*>(mixedImage.bits());
  const int mixed_stride = mixedImage.bytesPerLine() / sizeof(uint32_t);
  const auto* foreground_line = foreground.bits();
  const int foreground_stride = foreground.bytesPerLine();
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  const QVector<QRgb> foreground_palette = foreground.colorTable();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask_line[x >> 5] & (msb >> (x & 31))) {
        uint32_t color = foreground_palette[foreground_line[x]];
        mixed_line[x] = color;
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
    mask_line += mask_stride;
  }
}

template <>
void combineImagesColor<uint8_t, uint8_t>(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixed_line = mixedImage.bits();
  const int mixed_stride = mixedImage.bytesPerLine();
  const auto* foreground_line = foreground.bits();
  const int foreground_stride = foreground.bytesPerLine();
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  QVector<uint32_t> mixed_palette = mixedImage.colorTable();
  const QVector<uint32_t> foreground_palette = foreground.colorTable();
  if (mixed_palette.size() < 256) {
    mergePalettes(mixed_palette, foreground_palette);
    if (mixed_palette.size() > 256) {
      mixed_palette.resize(256);
    }
    mixedImage.setColorTable(mixed_palette);
  }

  std::unordered_map<uint32_t, uint8_t> colorToIndex;
  for (int i = 0; i < mixed_palette.size(); ++i) {
    colorToIndex[mixed_palette[i]] = static_cast<uint8_t>(i);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask_line[x >> 5] & (msb >> (x & 31))) {
        uint32_t color = foreground_palette[foreground_line[x]];
        mixed_line[x] = colorToIndex[color];
      }
    }
    mixed_line += mixed_stride;
    foreground_line += foreground_stride;
    mask_line += mask_stride;
  }
}

template <typename MixedPixel>
void applyMask(QImage& image, const BinaryImage& bw_mask, const BWColor filling_color = WHITE) {
  auto* image_line = reinterpret_cast<MixedPixel*>(image.bits());
  const int image_stride = image.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* bw_mask_line = bw_mask.data();
  const int bw_mask_stride = bw_mask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;
  const auto fillingPixel = static_cast<MixedPixel>((filling_color == WHITE) ? 0xffffffff : 0x00000000);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bw_mask_line[x >> 5] & (msb >> (x & 31)))) {
        image_line[x] = fillingPixel;
      }
    }
    image_line += image_stride;
    bw_mask_line += bw_mask_stride;
  }
}
}  // namespace impl

namespace {
inline void checkImageFormatSupported(const QImage& image) {
  if ((image.format() != QImage::Format_Indexed8) && (image.format() != QImage::Format_RGB32)
      && (image.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("ImageCombination: wrong image format.");
  }
}

template <typename T1, typename T2>
inline void checkImagesHaveEqualSize(const T1& image1, const T2& image2) {
  if (image1.size() != image2.size()) {
    throw std::invalid_argument("ImageCombination: images size don't match.");
  }
}

template <typename T1, typename T2, typename T3>
inline void checkImagesHaveEqualSize(const T1& image1, const T2& image2, const T3& image3) {
  if ((image1.size() != image2.size()) || (image1.size() != image3.size())) {
    throw std::invalid_argument("ImageCombination: images size don't match.");
  }
}
}  // namespace

namespace impl {
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    combineImagesMono<uint8_t>(mixedImage, foreground);
  } else {
    combineImagesMono<uint32_t>(mixedImage, foreground);
  }
}

void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    combineImagesMono<uint8_t>(mixedImage, foreground, mask);
  } else {
    combineImagesMono<uint32_t>(mixedImage, foreground, mask);
  }
}

void combineImagesColor(QImage& mixedImage, const QImage& foreground) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    if (mixedImage.isGrayscale() && foreground.isGrayscale()) {
      combineImagesColor<uint8_t>(mixedImage, foreground);
    } else {
      combineImagesColor<uint8_t, uint8_t>(mixedImage, foreground);
    }
  } else {
    if ((foreground.format() == QImage::Format_RGB32) || (foreground.format() == QImage::Format_ARGB32)) {
      combineImagesColor<uint32_t>(mixedImage, foreground);
    } else {
      combineImagesColor<uint32_t, uint8_t>(mixedImage, foreground);
    }
  }
}

void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    if (mixedImage.isGrayscale() && foreground.isGrayscale()) {
      combineImagesColor<uint8_t>(mixedImage, foreground, mask);
    } else {
      combineImagesColor<uint8_t, uint8_t>(mixedImage, foreground, mask);
    }
  } else {
    if ((foreground.format() == QImage::Format_RGB32) || (foreground.format() == QImage::Format_ARGB32)) {
      combineImagesColor<uint32_t>(mixedImage, foreground, mask);
    } else {
      combineImagesColor<uint32_t, uint8_t>(mixedImage, foreground, mask);
    }
  }
}

void applyMask(QImage& image, const BinaryImage& bw_mask, const BWColor filling_color) {
  if (image.format() == QImage::Format_Indexed8) {
    applyMask<uint8_t>(image, bw_mask, filling_color);
  } else {
    applyMask<uint32_t>(image, bw_mask, filling_color);
  }
}
}  // namespace impl

void combineImages(QImage& mixedImage, const BinaryImage& foreground) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  impl::combineImagesMono(mixedImage, foreground);
}

void combineImages(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  impl::combineImagesMono(mixedImage, foreground, mask);
}

void combineImages(QImage& mixedImage, const QImage& foreground) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  if ((foreground.format() == QImage::Format_Mono) || (foreground.format() == QImage::Format_MonoLSB)) {
    impl::combineImagesMono(mixedImage, BinaryImage(foreground));
  } else {
    impl::combineImagesColor(mixedImage, foreground);
  }
}

void combineImages(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground, mask);

  if ((foreground.format() == QImage::Format_Mono) || (foreground.format() == QImage::Format_MonoLSB)) {
    impl::combineImagesMono(mixedImage, BinaryImage(foreground), mask);
  } else {
    impl::combineImagesColor(mixedImage, foreground, mask);
  }
}

void applyMask(QImage& image, const BinaryImage& bw_mask, const BWColor filling_color) {
  checkImageFormatSupported(image);
  checkImagesHaveEqualSize(image, bw_mask);

  impl::applyMask(image, bw_mask, filling_color);
}
}  // namespace imageproc