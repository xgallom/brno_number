#include "number.hpp"

#include <algorithm>

using num_t = number::num_t;
using result_t = number::result_t;
using sresult_t = number::sresult_t;

static result_t rsum(num_t* dest, const num_t* left, const num_t* right, size_t count, result_t overflow = 0)
{
	while (count--) {
		const result_t sum = result_t(*left--) + result_t(*right--) + overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	}

	return overflow;
}

static sresult_t rsub(num_t* dest, const num_t* left, const num_t* right, size_t count, sresult_t overflow = 0)
{
	while (count--) {
		const sresult_t sum = sresult_t(*left--) - sresult_t(*right--) - overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	}

	return overflow;
}

static void rneg(num_t* num, size_t count, sresult_t overflow = 0)
{
	while (count--) {
		const sresult_t sum = -sresult_t(*num) - overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*num-- = num_t(sum & number::ResultMask);
	}
}

number::number(int value) {
	m_size = 1;

	if (value < 0) {
		m_data = { num_t(-value) };
		negate();
	}
	else
		m_data = { num_t(value) };
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
	if (rsum(result.rbegin(), left.rbegin(), right.rbegin(), size))
		result.pushFront(1);

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

	return out << std::dec << "]\n}";
}

int main()
{
	//number a(0x7fffffff), b(0x7fffffff), apb = a + b, apbpapb = apb + apb, apbpapbpa = apbpapb + a;
	number
		a(~2, 0, number::data_t({ 1, 1 })),
		b(~2, 0, number::data_t({ 1, 1 }));

	std::cout
		<< "a: " << a << "\n"
		<< "b: " << b << "\n"

		<< "a + b: " << (a + b) << "\n";

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

