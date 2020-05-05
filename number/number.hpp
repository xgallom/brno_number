#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

class number {
public:
	using num_t = uint32_t;
	using snum_t = int32_t;
	using result_t = uint64_t;
	using sresult_t = int64_t;
	using exp_t = int64_t;
	using numsize_t = size_t;
	using capacity_t = size_t;
	using precision_t = capacity_t;
	using data_t = std::vector<num_t>;

	static constexpr precision_t DefaultPrecision = 0;
	static constexpr capacity_t DefaultCapacity = 1 + DefaultPrecision;
	// MSB is the sign flag
	static constexpr numsize_t SignFlag = numsize_t(0x1) << (std::numeric_limits<numsize_t>::digits - 1);
	// Result in lower DWORD, Overflow in upper DWORD
	static constexpr result_t OverflowOffset = std::numeric_limits<result_t>::digits >> 1;
	static constexpr result_t ResultMask = std::numeric_limits<result_t>::max() >> OverflowOffset;
	static constexpr result_t OverflowMask = ~ResultMask;

private:
	numsize_t m_size = 1;
	exp_t m_exponent = 0;
	data_t m_data = { 0 };

public:
	number() = default;
	number(const number&) = default;
	number(number&&) = default;
	number& operator=(const number&) = default;
	number& operator=(number&&) = default;

	number(int value);
	inline explicit number(numsize_t size, exp_t exponent, data_t data)
		: m_size(size), m_exponent(exponent), m_data(data)
	{}

	inline numsize_t size() const { return isNegative() ? ~m_size : m_size; }
	inline exp_t exponent() const { return m_exponent; }
	inline exp_t minimumExponent() const { return m_exponent - size(); }
	inline capacity_t capacity() const { return m_data.capacity(); }
	inline precision_t precision() const { return capacity() - 1; }
	inline bool isNegative() const { return m_size & SignFlag; }
	inline bool isPositive() const { return !isNegative(); }
	inline bool isNotEmpty() const { return bool(size()); }

	number& negate();
	number operator-() const;

	friend number operator+(number left, number right);
	friend number operator*(const number& left, const number& right);

	inline const num_t& operator[](size_t offset) const { return m_data[offset]; }

	explicit operator bool() const;
	
	void truncate();

protected:
	inline number(numsize_t size, exp_t exponent)
		: m_size(size), m_exponent(exponent)
	{
		reserveSize();
	}

	void reserve(capacity_t capacity);
	void reserveSize();
	void pushFront(num_t value = 0, size_t count = 1);
	void pushBack(num_t value = 0, size_t count = 1);

	void turnNegative();

public:
	inline num_t* begin() { return m_data.data(); }
	inline const num_t* begin() const { return m_data.data(); }

	inline num_t* end() { return begin() + size(); }
	inline const num_t* end() const { return begin() + size(); }

	inline num_t* nth(numsize_t n) { return begin() + n; }
	inline const num_t* nth(numsize_t n) const { return begin() + n - 1; }

	inline num_t* rbegin() { return end() - 1; }
	inline const num_t* rbegin() const { return end() - 1; }

	inline num_t* rend() { return begin() - 1; }
	inline const num_t* rend() const { return begin() - 1; }

protected:
	// Add/subtract two positive numbers
	static number addPositive(number&& left, number&& right);
	static number subPositive(number&& left, number&& right);
	static number multiply(const number& left, const number& right);
};

std::ostream& operator<<(std::ostream& out, const number& value);
