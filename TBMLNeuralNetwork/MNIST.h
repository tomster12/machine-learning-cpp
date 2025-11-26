#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include "Tensor.h"

using uchar = unsigned char;

static inline uint32_t swap32(uint32_t x)
{
	// Cross-platform big-endian swap for 32-bit ints
	return ((x & 0x000000FFu) << 24) |
		((x & 0x0000FF00u) << 8) |
		((x & 0x00FF0000u) >> 8) |
		((x & 0xFF000000u) >> 24);
}

class MNIST
{
public:
	static uchar** readImages(const std::string& path, size_t& imageCount, size_t& imageSize)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Failed to open: " + path);

		uint32_t magic, count, rows, cols;

		file.read((char*)&magic, 4);
		magic = swap32(magic);
		if (magic != 2051) throw std::runtime_error("Invalid MNIST image file");

		file.read((char*)&count, 4);
		file.read((char*)&rows, 4);
		file.read((char*)&cols, 4);

		count = swap32(count);
		rows = swap32(rows);
		cols = swap32(cols);

		imageCount = count;
		imageSize = rows * cols;

		uchar** out = new uchar * [imageCount];

		for (size_t i = 0; i < imageCount; i++)
		{
			out[i] = new uchar[imageSize];
			file.read((char*)out[i], imageSize);
		}
		return out;
	}

	static uchar* readLabels(const std::string& path, size_t& labelCount)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Failed to open: " + path);

		uint32_t magic, count;

		file.read((char*)&magic, 4);
		magic = swap32(magic);
		if (magic != 2049) throw std::runtime_error("Invalid MNIST label file");

		file.read((char*)&count, 4);
		count = swap32(count);

		labelCount = count;

		uchar* labels = new uchar[labelCount];
		file.read((char*)labels, labelCount);

		return labels;
	}

	static tbml::Tensor readImagesTensor(const std::string& path, size_t& imageCount, size_t& imageSize)
	{
		uchar** images = readImages(path, imageCount, imageSize);

		tbml::Tensor t({ imageCount, imageSize }, 0);

		for (size_t i = 0; i < imageCount; i++)
		{
			for (size_t j = 0; j < imageSize; j++)
				t(i, j) = images[i][j] / 255.0f;
			delete[] images[i];
		}

		delete[] images;
		return t;
	}

	static tbml::Tensor readLabelsTensor(const std::string& path, size_t& count)
	{
		uchar* labels = readLabels(path, count);

		tbml::Tensor t({ count, 10 }, 0);
		for (size_t i = 0; i < count; i++)
			t(i, labels[i]) = 1.0f;

		delete[] labels;
		return t;
	}
};
