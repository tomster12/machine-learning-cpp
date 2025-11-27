#pragma once

#include <functional>

namespace tbml
{
	// Column-major order vector<float> based tensor
	// e.g. shape[0] = rows, shape[1] = columns, ...
	class Tensor
	{
	public:
		static const Tensor ZERO;

		Tensor();
		Tensor(const Tensor& t);
		Tensor(const std::vector<size_t>& shape, float v);
		Tensor(const std::vector<size_t>& shape, const std::vector<float>& data);
		Tensor(const std::vector<float>& data);
		Tensor(const std::vector<std::vector<float>>& data);
		Tensor(const std::vector<std::vector<std::vector<float>>>& data);
		void zero();
		void set(std::vector<size_t>&& shape, std::vector<float>&& data);
		void set(const std::vector<size_t>& shape, float v);
		void set(const std::vector<size_t>& shape);

		template<typename... Args>
		float& at(Args... args) { return data[_getIndex(0, 1, args...)]; }

		template<typename... Args>
		float at(Args... args) const { return data[_getIndex(0, 1, args...)]; }

		template<typename... Args>
		float& operator()(Args... args) { return at(args...); }

		template<typename... Args>
		float operator()(Args... args) const { return at(args...); }

		Tensor& add(const Tensor& t);
		Tensor& add(const Tensor& t, size_t moddim);
		Tensor& add(float v);
		Tensor& sub(const Tensor& t);
		Tensor& sub(float v);
		Tensor& mult(const Tensor& t);
		Tensor& mult(float v);
		Tensor& div(const Tensor& t);
		Tensor& div(float v);
		float acc(std::function<float(float, float)> fn, float initial) const;
		Tensor& map(std::function<float(float)> fn);
		Tensor& ewise(const Tensor& t, std::function<float(float, float)> fn);
		Tensor& matmul(const Tensor& t);
		Tensor& transpose();

		Tensor mapped(std::function<float(float)> fn) const { return Tensor(*this).map(fn); }
		Tensor ewised(const Tensor& t, std::function<float(float, float)> fn) const { return Tensor(*this).ewise(t, fn); }
		Tensor matmulled(const Tensor& t) const { return Tensor(*this).matmul(t); }
		Tensor transposed() const { return Tensor(*this).transpose(); }
		Tensor sample(size_t dim, std::vector<size_t> indices) const;

		Tensor& matmulled_to(const Tensor& t, Tensor& out) const;

		Tensor& operator+=(const Tensor& t) { return add(t); }
		Tensor& operator+=(float v) { return add(v); }
		Tensor& operator-=(const Tensor& t) { return sub(t); }
		Tensor& operator-=(float v) { return sub(v); }
		Tensor& operator*=(const Tensor& t) { return mult(t); }
		Tensor& operator*=(float v) { return mult(v); }
		Tensor& operator/=(const Tensor& t) { return div(t); }
		Tensor& operator/=(float v) { return div(v); }
		Tensor operator+(const Tensor& t) const { return Tensor(*this).add(t); }
		Tensor operator+(float v) const { return Tensor(*this).add(v); }
		Tensor operator-(const Tensor& t) const { return Tensor(*this).sub(t); }
		Tensor operator-(float v) const { return Tensor(*this).sub(v); }
		Tensor operator*(const Tensor& t) const { return Tensor(*this).mult(t); }
		Tensor operator*(float v) const { return Tensor(*this).mult(v); }
		Tensor operator/(const Tensor& t) const { return Tensor(*this).div(t); }
		Tensor operator/(float v) const { return Tensor(*this).div(v); }

		void print(std::string tag = "Tensor:") const;
		std::vector<Tensor> groupRows(size_t targetGroupSize) const;
		const std::vector<size_t> getShape() const { return shape; }
		const size_t getShape(size_t dim) const { return dim <= shape.size() ? shape[dim] : 1; }
		const size_t getDims() const { return shape.size(); }
		const size_t getSize() const { return data.size(); }
		const std::vector<float>& getData() const { return data; }
		bool isZero() const;

		void serialize(std::ostream& os) const;
		static Tensor deserialize(std::istream& is);

	private:
		std::vector<size_t> shape;
		std::vector<float> data;

		template<typename ICurrent, typename... IRest>
		size_t _getIndex(size_t acc, size_t mult, ICurrent index, IRest... rest) const
		{
			// t[a, b, c] = data[a + b * shape[0] + c * shape[0] * shape[1]]
			return _getIndex(acc + (index * mult), (mult * shape[shape.size() - sizeof...(IRest) - 1]), rest...);
		}

		size_t _getIndex(size_t acc, size_t mult) const { return acc; }
	};
}
