#include <omp.h>
#include <cassert>
#include "stdafx.h"
#include "Tensor.h"
#include "TbmlGlobal.h"

namespace tbml
{
	const Tensor Tensor::ZERO = Tensor();

	Tensor::Tensor()
	{
		shape = {};
		data = {};
	}

	Tensor::Tensor(const Tensor& t)
	{
		shape = t.shape;
		data = t.data;
	}

	Tensor& Tensor::operator=(const Tensor& t)
	{
		shape = t.shape;
		data = t.data;
		return *this;
	}

	Tensor::Tensor(Tensor&& t) noexcept
	{
		this->shape = std::move(t.shape);
		this->data = std::move(t.data);
	}

	Tensor& Tensor::operator=(Tensor&& t) noexcept
	{
		this->shape = std::move(t.shape);
		this->data = std::move(t.data);
		return *this;
	}

	Tensor::Tensor(const std::vector<size_t>& shape, float v)
	{
		size_t dataSize = 1;
		for (size_t i = 0; i < shape.size(); i++) dataSize *= shape[i];

		this->shape = shape;
		this->data = std::vector<float>(dataSize, v);
	}

	Tensor::Tensor(const std::vector<size_t>& shape, const std::vector<float>& data)
	{
		size_t dataSize = 1;
		for (size_t i = 0; i < shape.size(); i++) dataSize *= shape[i];

		assert(dataSize == data.size());

		this->shape = shape;
		this->data = data;
	}

	Tensor::Tensor(const std::vector<float>& data)
	{
		// Hardcoded 1D overload
		this->shape = { data.size() };
		this->data = data;
	}

	Tensor::Tensor(const std::vector<std::vector<float>>& data)
	{
		// Hardcoded 2D overload
		shape = { data.size(), data[0].size() };
		this->data = std::vector<float>(shape[0] * shape[1]);
		for (size_t i = 0; i < shape[0]; i++)
		{
			for (size_t j = 0; j < shape[1]; j++)
			{
				this->at(i, j) = data[i][j];
			}
		}
	}

	Tensor::Tensor(const std::vector<std::vector<std::vector<float>>>& data)
	{
		// Hardcoded 3D overload
		shape = { data[0].size(), data[0].size(), data[0][0].size() };
		this->data = std::vector<float>(shape[0] * shape[1] * shape[2]);
		for (size_t i = 0; i < shape[0]; i++)
		{
			for (size_t j = 0; j < shape[1]; j++)
			{
				for (size_t k = 0; k < shape[2]; k++)
				{
					this->at(i, j, k) = data[i][j][k];
				}
			}
		}
	}

	void Tensor::moveData(std::vector<size_t>&& shape, std::vector<float>&& data)
	{
		this->shape = std::move(shape);
		this->data = std::move(data);
	}

	void Tensor::resizeData(const std::vector<size_t>& shape)
	{
		if (this->shape == shape) return;

		size_t dataSize = 1;
		for (size_t i = 0; i < shape.size(); i++) dataSize *= shape[i];

		this->shape = shape;
		data.resize(dataSize);
	}

	void Tensor::setData(const std::vector<float>& src)
	{
		assert(src.size() == data.size());
		std::copy(src.begin(), src.end(), data.begin());
	}

	void Tensor::setData(std::initializer_list<float> src)
	{
		assert(src.size() == data.size());
		std::copy(src.begin(), src.end(), data.begin());
	}

	void Tensor::zero()
	{
		for (size_t i = 0; i < data.size(); i++) data[i] = 0;
	}

	Tensor& Tensor::add(const Tensor& t)
	{
		if (getDims() == 0)
		{
			shape = t.shape;
			data = t.data;
			return *this;
		}

		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] += t.data[i];
		return *this;
	}

	Tensor& Tensor::add(const Tensor& t, size_t moddim)
	{
		// Hardcoded only for 2D tensor and moddim < 2
		assert(moddim < 2);

		const size_t rows = shape[0];
		const size_t cols = shape[1];

		// Ensure that all dimensions except moddim are the same
		for (size_t i = 0; i < getDims(); i++)
		{
			if (i != moddim) assert(shape[i] == t.shape[i]);
		}

		// Example shape = (3, 4, 2)
		// [ 0, 3, 6, 9  ] .. [ 12, 15, 18, 21 ]
		// [ 1, 4, 7, 10 ] .. [ 13, 16, 19, 22 ]
		// [ 2, 5, 8, 11 ] .. [ 14, 17, 20, 23 ]

		if (moddim == 0)
		{
			// t.shape = (1, 4, 2) => Take all the data to closest row 0
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]

			for (size_t i = 0; i < rows; i++)
			{
				for (size_t j = 0; j < cols; j++)
				{
					const float colVal = t.data[j];
					data[i * cols + j] += colVal;
				}
			}
		}

		else if (moddim == 1)
		{
			// t,shape = (3, 1, 2) => Take all the data to closest col 0
			// [ 0, 0, 0, 0 ] .. [ 3, 3, 3, 3 ]
			// [ 1, 1, 1, 1 ] .. [ 4, 4, 4, 4 ]
			// [ 2, 2, 2, 2 ] .. [ 5, 5, 5, 5 ]

			for (size_t i = 0; i < rows; i++)
			{
				const float rowVal = t.data[i];
				for (size_t j = 0; j < cols; j++)
				{
					data[i * cols + j] += rowVal;
				}
			}
		}

		return *this;
	}

	Tensor& Tensor::add(float v)
	{
		for (size_t i = 0; i < data.size(); i++) data[i] += v;
		return *this;
	}

	Tensor& Tensor::sub(const Tensor& t)
	{
		if (getDims() == 0)
		{
			shape = t.shape;
			data = t.data;
			for (size_t i = 0; i < data.size(); i++) data[i] = -data[i];
			return *this;
		}

		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] -= t.data[i];
		return *this;
	}

	Tensor& Tensor::sub(float v)
	{
		for (size_t i = 0; i < data.size(); i++) data[i] -= v;
		return *this;
	}

	Tensor& Tensor::mult(const Tensor& t)
	{
		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] *= t.data[i];
		return *this;
	}

	Tensor& Tensor::mult(float v)
	{
		for (size_t i = 0; i < data.size(); i++) data[i] *= v;
		return *this;
	}

	Tensor& Tensor::div(const Tensor& t)
	{
		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] /= t.data[i];
		return *this;
	}

	Tensor& Tensor::div(float v)
	{
		for (size_t i = 0; i < data.size(); i++) data[i] /= v;
		return *this;
	}

	float Tensor::acc(std::function<float(float, float)> fn, float initial) const
	{
		float acc = initial;
		for (size_t i = 0; i < data.size(); i++) acc = fn(data[i], acc);
		return acc;
	}

	Tensor& Tensor::map(std::function<float(float)> fn)
	{
		for (size_t i = 0; i < data.size(); i++) data[i] = fn(data[i]);
		return *this;
	}

	Tensor& Tensor::map_to(std::function<float(float)> fn, Tensor& out) const
	{
		if (out.shape != shape) out.resizeData(shape);
		for (size_t i = 0; i < data.size(); i++) out.data[i] = fn(data[i]);
		return out;
	}

	Tensor& Tensor::ewise(const Tensor& t, std::function<float(float, float)> fn)
	{
		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] = fn(data[i], t.data[i]);
		return *this;
	}

	Tensor& Tensor::matmul(const Tensor& t)
	{
		const size_t dims = getDims();
		assert(dims == t.getDims());

		std::vector<float>& a = data;
		const std::vector<float>& b = t.data;

		if (dims == 1)
		{
			const size_t aCols = shape[0];
			const size_t bCols = t.shape[0];
			assert(aCols == bCols);

			for (size_t i = 0; i < data.size(); i++) a[i] *= b[i];
			return *this;
		}

		else if (dims == 2)
		{
			const size_t aRows = shape[0];
			const size_t aCols = shape[1];
			const size_t bRows = t.shape[0];
			const size_t bCols = t.shape[1];
			assert(aCols == bRows);

			std::vector<float> out(aRows * bCols);

			int threads = tbml::getOmpThreads();
			#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < aRows; i++)
			{
				const float* aRow = &a[i * aCols];
				float* outRow = &out[i * bCols];

				for (int j = 0; j < bCols; j++)
				{
					const float* bCol = &b[j];
					const float* aPtr = aRow;
					const float* bPtr = bCol;

					float acc = 0.0f;
					for (int k = 0; k < aCols; k++)
					{
						acc += (*aPtr) * (*bPtr);
						aPtr += 1;
						bPtr += bCols;
					}

					outRow[j] = acc;
				}
			}

			data.swap(out);
			shape[1] = t.shape[1];
			return *this;
		}

		throw std::runtime_error("Invalid shape for matrix multiplication");
	}

	Tensor& Tensor::matmul_to(const Tensor& t, Tensor& out) const
	{
		const size_t dims = getDims();
		assert(dims == t.getDims());

		const std::vector<float>& a = data;
		const std::vector<float>& b = t.data;
		std::vector<float>& outData = out.data;

		if (dims == 1)
		{
			const size_t aCols = getShape(0);
			const size_t bCols = t.getShape(0);
			assert(aCols == bCols);

			if (out.getDims() != 2 || out.shape[0] != aCols) out.resizeData({ aCols });

			for (size_t i = 0; i < aCols; i++) outData[i] = a[i] * b[i];
		}

		else if (dims == 2)
		{
			const size_t aRows = shape[0];
			const size_t aCols = shape[1];
			const size_t bRows = t.shape[0];
			const size_t bCols = t.shape[1];
			assert(aCols == bRows);

			if (out.getDims() != 2 || out.shape[0] != aRows || out.shape[1] != bCols) out.resizeData({ aRows, bCols });

			int threads = tbml::getOmpThreads();
			#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < aRows; i++)
			{
				const float* aRow = &a[i * aCols];
				float* outRow = &outData[i * bCols];

				for (int j = 0; j < bCols; j++)
				{
					const float* bCol = &b[j];
					const float* aPtr = aRow;
					const float* bPtr = bCol;

					float acc = 0.0f;
					for (int i = 0; i < aCols; i++)
					{
						acc += (*aPtr) * (*bPtr);
						aPtr += 1;
						bPtr += bCols;
					}

					outRow[j] = acc;
				}
			}
		}

		return out;
	}

	Tensor& Tensor::transpose()
	{
		if (getDims() == 1)
		{
			const size_t rows = shape[0];
			shape = { 1, rows };
			return *this;
		}

		else if (getDims() == 2)
		{
			const size_t rows = shape[0];
			const size_t cols = shape[1];
			std::vector<float> out(rows * cols);

			for (size_t i = 0; i < rows; i++)
			{
				for (size_t j = 0; j < cols; j++)
				{
					out[j * rows + i] = data[i * cols + j];
				}
			}

			data = std::move(out);
			shape = { cols, rows };
			return *this;
		}

		throw std::runtime_error("Transpose not defined for dim > 2");
	}

	Tensor Tensor::sample(size_t dim, std::vector<size_t> indices) const
	{
		// Hardcoded only for 2D tensor and dim 0
		assert(getDims() == 2);
		assert(dim == 0);

		const size_t rows = shape[0];
		const size_t cols = shape[1];

		std::vector<float> out(indices.size() * cols);
		for (size_t i = 0; i < indices.size(); i++)
		{
			size_t srcI = indices[i];
			const float* row = &data[srcI * cols];
			float* outRow = &out[i * cols];

			for (size_t j = 0; j < cols; j++)
			{
				outRow[j] = row[j];
			}
		}

		return Tensor({ indices.size(), cols }, out);
	}

	void Tensor::print(std::string tag) const
	{
		std::cout << tag << std::endl;

		std::string shapeStr;
		for (size_t i = 0; i < getDims(); i++) shapeStr += std::to_string(shape[i]) + " ";
		std::cout << "\t( " << shapeStr << ")" << std::endl;

		std::string dataStr;

		if (getDims() == 1)
		{
			if (data.size() > 50) dataStr += "\t[ ... ]";
			else
			{
				dataStr += "\t[ ";
				for (size_t i = 0; i < data.size(); i++) dataStr += std::to_string(data[i]) + " ";
				dataStr += "]";
			}
		}

		else if (getDims() == 2)
		{
			if (data.size() > 50) dataStr += "\t[ ... ]";
			else
			{
				for (size_t i = 0; i < shape[0]; i++)
				{
					dataStr += "\t[ ";
					for (size_t j = 0; j < shape[1]; j++)
					{
						dataStr += std::to_string(data[i * shape[1] + j]) + " ";
					}
					dataStr += "]\n";
				}
			}
		}

		std::cout << dataStr << std::endl;
	}

	std::vector<Tensor> Tensor::groupRows(size_t targetGroupSize) const
	{
		// Hardcoded only for 2D tensor
		assert(getDims() == 2);

		const size_t rows = shape[0];
		const size_t cols = shape[1];

		size_t groupCount = (size_t)(ceil((float)rows / targetGroupSize));
		std::vector<Tensor> groups(groupCount);

		bool hasUneven = (rows % targetGroupSize) != 0;
		for (size_t g = 0; g < groupCount; g++)
		{
			size_t groupSize = (hasUneven && (g == groupCount - 1)) ? (rows % targetGroupSize) : targetGroupSize;
			groups[g].resizeData({ groupSize, cols });

			for (size_t i = 0; i < groupSize; i++)
			{
				size_t srcI = g * targetGroupSize + i;
				for (size_t j = 0; j < cols; j++)
				{
					groups[g].data[i * cols + j] = data[srcI * cols + j];
				}
			}
		}

		return groups;
	}

	void Tensor::serialize(std::ostream& os) const
	{
		os << "Tensor\n";
		os << getDims() << "\n";
		for (size_t i = 0; i < getDims(); i++) os << shape[i] << " ";
		os << "\n";
		for (size_t i = 0; i < data.size(); i++) os << data[i] << " ";
		os << "\n";
	}

	Tensor Tensor::deserialize(std::istream& is)
	{
		std::string type;
		size_t dims;
		std::vector<size_t> shape;
		std::vector<float> data;

		is >> type;
		is >> dims;
		shape = std::vector<size_t>(dims);
		size_t size = 1;
		for (size_t i = 0; i < dims; i++)
		{
			is >> shape[i];
			size *= shape[i];
		}
		data = std::vector<float>(size);
		for (size_t i = 0; i < size; i++) is >> data[i];
		return Tensor(shape, data);
	}
}
