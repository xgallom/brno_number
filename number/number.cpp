#include "number.hpp"

#include <algorithm>
#include <iomanip>

using num_t = number::num_t;
using result_t = number::result_t;
using sresult_t = number::sresult_t;
using data_t = number::data_t;

// Compute a recursive addition over buffers left and right into dest, and return overflow
// dest, left, right size >= count
// returns 0 or 1
static result_t radd(num_t* __restrict dest, const num_t* __restrict left, const num_t* __restrict right, size_t count, result_t overflow = 0)
{
	do {
		const result_t sum = result_t(*left--) + result_t(*right--) + overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	} while (--count);

	return overflow;
}

// Compute a recursive addition of buffers src and dest into dest itself, and return overflow
// dest, src size >= count
// returns 0 or 1
static result_t rsum(num_t* __restrict dest, const num_t* __restrict src, size_t count, result_t overflow = 0)
{
	do {
		const result_t sum = result_t(*dest) + result_t(*src--) + overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	} while (--count);

	return overflow;
}

// Compute a recursive difference over buffers left and right into dest, and return overflow
// dest, left, right size >= count
// returns 0 or 1
static sresult_t rsub(num_t* __restrict dest, const num_t* __restrict left, const num_t* __restrict right, size_t count, sresult_t overflow = 0)
{
	do {
		const sresult_t sum = sresult_t(*left--) - sresult_t(*right--) - overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	} while (--count);

	return overflow;
}

// Compute a recursive negative value of a buffer num over the size of count into the buffer itself
// num size >= count
static void rneg(num_t* __restrict num, size_t count, sresult_t overflow = 0)
{
	do {
		const sresult_t sum = -sresult_t(*num) - overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*num-- = num_t(sum & number::ResultMask);
	} while (--count);
}

// Compute a recursive multiplication of a buffer src by a number value into dest
// dest, src size >= count
static num_t rmul(num_t* __restrict dest, const num_t* __restrict src, const num_t value, size_t count, result_t overflow = 0)
{
	const result_t r_value = value;

	do {
		const result_t sum = result_t(*src--) * r_value + overflow;

		overflow = sum >> number::OverflowOffset;
		*dest-- = num_t(sum & number::ResultMask);
	} while (--count);

	return overflow;
}

// Compute a recursive multiplication of buffers bigger and smaller into dest
// Recommended biggerSize >= smallerSize
// dest size >= biggerSize + smallerSize + 1
// bigger size >= biggerSize
// smaller size >= smallerSize
static void rmul(num_t* __restrict dest, const num_t* __restrict bigger, size_t biggerSize, const num_t* __restrict smaller, size_t smallerSize)
{
	// Create a local buffer for partial multiplication
	const auto mulBufferSize = biggerSize + 1;
	data_t multiplicationBuffer(mulBufferSize + 1, 0);

	num_t
		& mulBufferOverflow = multiplicationBuffer[0],
		* const mulBuffer = multiplicationBuffer.data() + biggerSize;

	do {
		num_t shiftOffset = 0;
		result_t overflow = 0;

		mulBufferOverflow = rmul(mulBuffer, bigger, *smaller--, biggerSize);
		*(dest - mulBufferSize) += rsum(dest, mulBuffer, mulBufferSize);

		--dest;
	} while (--smallerSize);
}

// Construct a number from an int value
number::number(int value) {
	if (value < 0) {
		m_data[0] = -value;
		negate();
	}
	else
		m_data[0] = value;
}

void number::reserve(capacity_t capacity)
{
	m_data.resize(capacity, 0);
}

void number::reserveSize()
{
	reserve(size());
}

void number::pushFront(num_t value, size_t count)
{
	pushBack(value, count);
	std::rotate(m_data.begin(), m_data.end() - count, m_data.end());
}

void number::pushBack(num_t value, size_t count)
{
	m_size += count;
	m_exponent += count;
	m_data.reserve(m_size);

	while (count--)
		m_data.push_back(value);
}

void number::truncate()
{
	const auto
		front = std::find_if(m_data.begin(), m_data.end(), [](const auto& value) { return value; }),
		back = std::find_if(m_data.rbegin(), std::make_reverse_iterator(front), [](const auto& value) { return value; }).base();

	if (front >= back)
		*this = {};
	else {
		const auto size = std::distance(front, back);

		std::rotate(m_data.begin(), front, back);
		m_exponent -= std::distance(m_data.begin(), front);
		m_size = isNegative() ? ~size : size;
		m_data.erase(front, m_data.end());
	}
}

void number::turnNegative()
{
	rneg(rbegin(), size());
	negate();
}

number& number::negate()
{
	m_size = ~m_size;
	return *this;
}

number number::operator-() const
{
	number result = *this;
	result.negate();
	return result;
}

number operator+(number left, number right)
{
	if (left.isPositive()) {
		if (right.isPositive())
			return number::addPositive(std::move(left), std::move(right));
		else
			return number::subPositive(std::move(left), std::move(right.negate()));
	}
	else {
		if (right.isPositive())
			return number::subPositive(std::move(right), std::move(left.negate()));
		else
			return number::addPositive(std::move(left.negate()), std::move(right.negate())).negate();
	}
}

number operator*(const number& left, const number& right)
{
	return number::multiply(left, right);
}

number::operator bool() const
{
	return size() != 1 || m_data[0];
}

number number::addPositive(number&& left, number&& right)
{
	// Determine correct boundaries
	const bool
		leftIsUpper = left.exponent() > right.exponent(),
		leftIsLower = left.minimumExponent() < right.minimumExponent();

	number
		& upper = leftIsUpper ? left : right,
		& notUpper = leftIsUpper ? right : left,
		& lower = leftIsLower ? left : right,
		& notLower = leftIsLower ? right : left;

	// Create result
	const auto size = upper.exponent() - lower.minimumExponent();
	number result(size, upper.exponent());

	// Equalize numbers
	notUpper.pushFront(0, upper.exponent() - notUpper.exponent());
	notLower.pushBack(0, notLower.minimumExponent() - lower.minimumExponent());

	// Sum and account for overflow
	if (radd(result.rbegin(), left.rbegin(), right.rbegin(), size))
		result.pushFront(1);

	result.truncate();

	return result;
}

number number::subPositive(number&& left, number&& right)
{
	// Determine correct boundaries
	const bool
		leftIsUpper = left.exponent() > right.exponent(),
		leftIsLower = left.minimumExponent() < right.minimumExponent();

	number
		& upper = leftIsUpper ? left : right,
		& notUpper = leftIsUpper ? right : left,
		& lower = leftIsLower ? left : right,
		& notLower = leftIsLower ? right : left;

	// Create result
	const auto size = upper.exponent() - lower.minimumExponent();
	number result(size, upper.exponent());

	// Equalize numbers
	notUpper.pushFront(0, upper.exponent() - notUpper.exponent());
	notLower.pushBack(0, notLower.minimumExponent() - lower.minimumExponent());

	// Sub and account for negative result on overflow
	if (rsub(result.rbegin(), left.rbegin(), right.rbegin(), size))
		result.turnNegative();

	result.truncate();

	return result;
}

number number::multiply(const number& left, const number& right)
{
	const bool
		leftIsBigger = left.size() > right.size();

	const number
		& bigger = leftIsBigger ? left : right,
		& smaller = leftIsBigger ? right : left;

	const auto size = left.size() + right.size() + 1;
	const auto isNegative = left.isNegative() ^ right.isNegative();

	number result(isNegative ? ~size : size, left.exponent() + right.exponent());

	rmul(result.rbegin(), bigger.rbegin(), bigger.size(), smaller.rbegin(), smaller.size());

	result.truncate();

	return result;
}

std::ostream& operator<<(std::ostream& out, const number& value)
{
	out << "{\n"
		<< "  sign: " << (value.isNegative() ? '-' : '+') << "\n"
		<< "  size: " << value.size() << "\n"
		<< "  expo: " << value.exponent() << "\n"
		<< "  cap : " << value.capacity() << "\n"
		<< "  data: [ " << std::hex;

	for (const auto& data : value)
		out << data << " ";

	out << "]\n"
		<< "        0x";

	for (const auto& data : value)
		out << std::setfill('0') << std::setw(8) << data;

	return out << std::dec << "\n}";
}

int main()
{
	//number a(0x7fffffff), b(0x7fffffff), apb = a + b, apbpapb = apb + apb, apbpapbpa = apbpapb + a;
	number
		a(2, 5, number::data_t({ 0xffffffff, 0xffffffff })),
		b(2, 2, number::data_t({ 0xffffffff, 0xffffffff }));

	std::cout
		<< "a: " << a << "\n"
		<< "b: " << b << "\n"

		<< "a * b: " << (a * b) << "\n";

	//		<< "a+b: " << apb << "\n"
			//<< "a+b+a+b: " << apbpapb << "\n"
			//<< "a+b+a+b+a: " << apbpapbpa << "\n";

		/*
		number c = a + b;
		number d = c / a;
		number e = d * a;
		assert(e == a + b);
		assert(e != a);
		assert(c > a);
		assert(a < b);
		assert(c.power(2) > c);
		c = number(10).power(-5);
		assert(c > c.power(2));

		return 0;*/

	return 0;
}

