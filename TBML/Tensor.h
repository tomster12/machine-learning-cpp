#pragma once

#include <functional>

namespace tbml
{
	// Row-major vector<float> tensor
	class Tensor
	{
	public:
		static const Tensor ZERO;

		Tensor();
		virtual ~Tensor() = default;
		Tensor(const Tensor& t);
		Tensor& operator=(const Tensor& t);
		Tensor(Tensor&& t) noexcept;
		Tensor& operator=(Tensor&& t) noexcept;

		Tensor(const std::vector<size_t>& shape, float v);
		Tensor(const std::vector<size_t>& shape, const std::vector<float>& data);
		Tensor(const std::vector<float>& data);
		Tensor(const std::vector<std::vector<float>>& data);
		Tensor(const std::vector<std::vector<std::vector<float>>>& data);
		void moveData(std::vector<size_t>&& shape, std::vector<float>&& data);
		void resizeData(const std::vector<size_t>& shape);
		void setData(const std::vector<float>& data);
		void setData(std::initializer_list<float> src);
		void zero();

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
		Tensor& map_to(std::function<float(float)> fn, Tensor& out) const;
		Tensor mapped(std::function<float(float)> fn) const { return Tensor(*this).map(fn); }
		Tensor& ewise(const Tensor& t, std::function<float(float, float)> fn);
		Tensor ewised(const Tensor& t, std::function<float(float, float)> fn) const { return Tensor(*this).ewise(t, fn); }
		Tensor& matmul(const Tensor& t);
		Tensor& matmul_to(const Tensor& t, Tensor& out) const;
		Tensor matmulled(const Tensor& t) const { return Tensor(*this).matmul(t); }
		Tensor& transpose();
		Tensor transposed() const { return Tensor(*this).transpose(); }
		Tensor sample(size_t dim, std::vector<size_t> indices) const;

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
		std::vector<float>& getData() { return data; }

		void serialize(std::ostream& os) const;
		static Tensor deserialize(std::istream& is);

		template<typename... Args>
		float& at(Args... args) { return data[idx(args...)]; }
		template<typename... Args>
		float at(Args... args) const { return data[idx(args...)]; }
		template<typename... Args>
		float& operator()(Args... args) { return at(args...); }
		template<typename... Args>
		float operator()(Args... args) const { return at(args...); }

	private:
		std::vector<size_t> shape;
		std::vector<float> data;

		inline size_t idx(size_t i) const { return i; }
		inline size_t idx(size_t i, size_t j) const { return j + i * shape[1]; }
		inline size_t idx(size_t i, size_t j, size_t k) const { return (j + i * shape[1]) * shape[2] + k; }
	};
}
