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
		// Default constructor
		shape = {};
		data = {};
	}

	Tensor::Tensor(const Tensor& t)
	{
		// Copy constructor
		shape = t.shape;
		data = t.data;
	}

	Tensor::Tensor(const std::vector<size_t>& shape, float v)
	{
		// Create tensor with shape and fill with v
		this->shape = shape;
		size_t dataSize = 1;
		for (size_t i = 0; i < getDims(); i++) dataSize *= shape[i];
		data = std::vector<float>(dataSize, v);
	}

	Tensor::Tensor(const std::vector<size_t>& shape, const std::vector<float>& data)
	{
		// Create tensor with shape and data and assert data fits
		this->shape = shape;
		size_t dataSize = 1;
		for (size_t i = 0; i < getDims(); i++) dataSize *= shape[i];
		assert(dataSize == data.size());
		this->data = data;
	}

	Tensor::Tensor(const std::vector<float>& data)
	{
		// Create 1D tensor
		this->shape = { data.size() };
		this->data = data;
	}

	Tensor::Tensor(const std::vector<std::vector<float>>& data)
	{
		// Create 2D tensor
		shape = { data.size(), data[0].size() };
		this->data = std::vector<float>(shape[0] * shape[1]);
		for (size_t row = 0; row < shape[0]; row++)
		{
			for (size_t col = 0; col < shape[1]; col++)
			{
				this->data[row + col * shape[0]] = data[row][col];
			}
		}
	}

	Tensor::Tensor(const std::vector<std::vector<std::vector<float>>>& data)
	{
		// Create 3D tensor
		shape = { data[0].size(), data[0][0].size(), data.size() };
		this->data = std::vector<float>(shape[0] * shape[1] * shape[2]);
		for (size_t x = 0; x < shape[0]; x++)
		{
			for (size_t y = 0; y < shape[1]; y++)
			{
				for (size_t z = 0; z < shape[2]; z++)
				{
					this->data[x + y * shape[0] + z * shape[0] * shape[1]] = data[z][x][y];
				}
			}
		}
	}

	void Tensor::zero()
	{
		for (size_t i = 0; i < data.size(); i++) data[i] = 0;
	}

	void Tensor::setData(std::vector<size_t>&& shape, std::vector<float>&& data)
	{
		// Set tensor with shape and data and assert data fits
		this->shape = std::move(shape);
		this->data = std::move(data);
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
		// Add function with broadcasting along moddim

		// TODO: Figure out the more generic way to do this
		assert(moddim < 2);
		for (size_t i = 0; i < getDims(); i++)
		{
			if (i != moddim) assert(shape[i] == t.shape[i]);
		}

		// shape = (3, 4, 2)
		// [ 0, 3, 6, 9  ] .. [ 12, 15, 18, 21 ]
		// [ 1, 4, 7, 10 ] .. [ 13, 16, 19, 22 ]
		// [ 2, 5, 8, 11 ] .. [ 14, 17, 20, 23 ]

		if (moddim == 0)
		{
			// shape = (1, 4, 2) => Take all the data to closest row 0
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]
			// [ 0, 1, 2, 3 ] .. [ 4, 5, 6, 7 ]
			// ni = i // 3
			for (size_t i = 0; i < data.size(); i++)
			{
				int ni = (int)(i / shape[0]);
				data[i] += t.data[ni];
			}
		}

		else if (moddim == 1)
		{
			// shape = (3, 1, 2) => Take all the data to closest col 0
			// [ 0, 0, 0, 0 ] .. [ 3, 3, 3, 3 ]
			// [ 1, 1, 1, 1 ] .. [ 4, 4, 4, 4 ]
			// [ 2, 2, 2, 2 ] .. [ 5, 5, 5, 5 ]
			for (size_t i = 0; i < data.size(); i++)
			{
				size_t ni = (i / (shape[0] * shape[1])) + (i % shape[0]);
				data[i] += t.data[ni];
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

	Tensor& Tensor::ewise(const Tensor& t, std::function<float(float, float)> fn)
	{
		assert(shape == t.shape);
		for (size_t i = 0; i < data.size(); i++) data[i] = fn(data[i], t.data[i]);
		return *this;
	}

	Tensor& Tensor::matmul(const Tensor& t)
	{
		if (getDims() == 1)
		{
			assert(getShape(0) == t.getShape(0));

			return this->operator*=(t);
		}

		else if (getDims() == 2)
		{
			assert(getShape(1) == t.getShape(0));

			const std::vector<float>& a = data;
			const std::vector<float>& b = t.data;
			std::vector<float> result(shape[0] * t.shape[1]);

			int threads = tbml::getOmpThreads();

			#pragma omp parallel for num_threads(threads)
			for (int row = 0; row < (int)shape[0]; row++)
			{
				for (int ocol = 0; ocol < (int)t.shape[1]; ocol++)
				{
					for (int i = 0; i < (int)shape[1]; i++)
					{
						// TODO: See if can use operator()
						result[row + shape[0] * ocol] += a[row + shape[0] * i] * b[i + t.shape[0] * ocol];
					}
				}
			}

			data = std::move(result);
			shape[1] = t.shape[1];
			return *this;
		}

		throw std::runtime_error("Invalid shape for matrix multiplication");
	}

	Tensor& Tensor::transpose()
	{
		if (getDims() == 1)
		{
			shape = { 1, shape[0] };
			return *this;
		}

		else if (getDims() == 2)
		{
			std::vector<float> result(shape[0] * shape[1]);

			for (size_t row = 0; row < shape[0]; row++)
			{
				for (size_t col = 0; col < shape[1]; col++)
				{
					result[col + shape[1] * row] = data[row + shape[0] * col];
				}
			}

			data = std::move(result);
			shape = { shape[1], shape[0] };
			return *this;
		}

		throw std::runtime_error("Transpose not defined for dim > 2");
	}

	Tensor Tensor::sample(size_t dim, std::vector<size_t> indices) const
	{
		assert(getDims() == 2);
		assert(dim == 0);

		// Only implemented for dim 0 of 2D tensor
		std::vector<float> result(indices.size() * shape[1]);
		for (size_t i = 0; i < indices.size(); i++)
		{
			for (size_t j = 0; j < shape[1]; j++)
			{
				result[i + indices.size() * j] = data[indices[i] + shape[0] * j];
			}
		}

		return Tensor({ indices.size(), shape[1] }, result);
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
				for (size_t x = 0; x < shape[0]; x++)
				{
					dataStr += "\t[ ";
					for (size_t y = 0; y < shape[1]; y++)
					{
						dataStr += std::to_string(data[x + shape[0] * y]) + " ";
					}
					dataStr += "]\n";
				}
			}
		}

		std::cout << dataStr << std::endl;
	}

	std::vector<Tensor> Tensor::groupRows(size_t targetGroupSize) const
	{
		// TODO: More generic way to do this
		assert(getDims() == 2);

		size_t groupCount = (size_t)(ceil((float)shape[0] / targetGroupSize));
		bool hasUneven = (shape[0] % targetGroupSize) != 0;

		std::vector<Tensor> groups = std::vector<Tensor>(groupCount);

		for (size_t i = 0; i < groupCount; i++)
		{
			size_t groupSize = (hasUneven && (i == groupCount - 1)) ? (shape[0] % targetGroupSize) : targetGroupSize;
			groups[i] = Tensor{ { groupSize, shape[1] }, 0 };

			for (size_t row = 0; row < groupSize; row++)
			{
				for (size_t col = 0; col < shape[1]; col++)
				{
					groups[i](row, col) = at(i * groupSize + row, col);
				}
			}
		}

		return groups;
	}

	bool Tensor::isZero() const
	{
		if (getDims() == 0) return true;
		for (size_t i = 0; i < getDims(); i++)
		{
			if (shape[i] != 0) return false;
		}
		return true;
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
