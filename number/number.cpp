#include "number.hpp"

#include <algorithm>
#include <iomanip>


//-TYPE-DEFINITIONS----------------------------------------------------------------------------------------------------

using sign_t = number::sign_t;
using Sign = number::Sign;

using num_t = number::num_t;
using snum_t = number::num_t;

using result_t = number::result_t;
using sresult_t = number::sresult_t;

using digits_t = number::digits_t;
using exp_t = number::exp_t;
using uexp_t = number::uexp_t;

using data_t = number::data_t;



//-BUFFER-ARITHMETIC-FUNCTIONS-----------------------------------------------------------------------------------------


// Computes a recursive addition over buffers left and right into dest, and returns overflow
//     dest, left, right size >= count
//     returns 0 or 1

static result_t radd(num_t *__restrict dest,
					 const num_t *__restrict left, const num_t *__restrict right, size_t count,
					 result_t overflow = 0
) noexcept
{
	do {
		const result_t sum = result_t(*left--) + result_t(*right--) + overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	} while(--count);

	return overflow;
}


// Computes a recursive subtraction over buffers left and right into dest, and returns overflow
//     dest, left, right size >= count
//     returns 0 or 1

static sresult_t rsub(num_t *__restrict dest,
					  const num_t *__restrict left, const num_t *__restrict right, size_t count,
					  sresult_t overflow = 0
) noexcept
{
	do {
		const sresult_t sum = sresult_t(*left--) - sresult_t(*right--) - overflow;
		const auto usum = result_t(sum);

		overflow = (usum & number::OverflowMask) ? 1 : 0;
		*dest-- = num_t(usum & number::ResultMask);
	} while(--count);

	return overflow;
}


// Computes a recursive addition of buffers src and dest into dest itself, and returns overflow
//     dest, src size >= count
//     returns 0 or 1

static result_t rsum(num_t *__restrict dest,
					 const num_t *__restrict src, size_t count,
					 result_t overflow = 0
) noexcept
{
	do {
		const result_t sum = result_t(*dest) + result_t(*src--) + overflow;

		overflow = sum & number::OverflowMask ? 1 : 0;
		*dest-- = num_t(sum & number::ResultMask);
	} while(--count);

	return overflow;
}


// Compute a recursive negative value of a buffer num over the size of count into the buffer itself
//     num size >= count

static void rneg(num_t *__restrict num, size_t count,
				 sresult_t overflow = 0
) noexcept
{
	do {
		const sresult_t sum = -sresult_t(*num) - overflow;
		const auto usum = result_t(sum);

		overflow = (usum & number::OverflowMask) ? 1 : 0;
		*num-- = num_t(usum & number::ResultMask);
	} while(--count);
}


// Compute a recursive multiplication of a buffer src by a number value into dest
//     dest, src size >= count

static num_t rmul(num_t *__restrict dest,
				  const num_t *__restrict src, size_t count,
				  const num_t value,
				  result_t overflow = 0
) noexcept
{
	const result_t r_value = value;

	do {
		const result_t sum = result_t(*src--) * r_value + overflow;

		overflow = sum >> number::OverflowOffset;
		*dest-- = num_t(sum & number::ResultMask);
	} while(--count);

	return num_t(overflow);
}


// Compute a recursive multiplication of buffers bigger and smaller into dest
//     biggerSize   >= smallerSize
//     dest size    >= biggerSize + smallerSize + 1
//     bigger size  >= biggerSize
//     smaller size >= smallerSize

static void rmul(num_t *__restrict dest,
				 const num_t *__restrict bigger, size_t biggerSize,
				 const num_t *__restrict smaller, size_t smallerSize
) noexcept
{
	// Create a local buffer for partial multiplication
	// TODO: Optimization is to avoid allocation in functions that call rmul recursively
	const auto mulBufferSize = biggerSize + 1;
	data_t multiplicationBuffer(mulBufferSize + 1, 0);

	num_t
			&mulBufferOverflow = multiplicationBuffer[0],
			*const mulBuffer = multiplicationBuffer.data() + biggerSize;

	do {
		mulBufferOverflow = rmul(mulBuffer, bigger, biggerSize, *smaller--);
		*(dest - mulBufferSize) += num_t(rsum(dest, mulBuffer, mulBufferSize));

		--dest;
	} while(--smallerSize);
}



//-VECTOR-ARITHMETIC-FUNCTIONS-----------------------------------------------------------------------------------------
//    These are the functions that operate on vectors and exponents, in abstraction they are between the number class
//    and low level recursive computations that try to never allocate
//    They mostly prepare buffers and handle control flow if the computation is more complex


// Basic vector operations

static inline num_t *rptr(data_t &vec) noexcept { return vec.data() + vec.size() - 1; }
static inline const num_t *rptr(const data_t &vec) noexcept { return vec.data() + vec.size() - 1; }
static inline exp_t minExp(exp_t exp, const data_t &vec) noexcept { return exp - exp_t(vec.size()); }

static inline exp_t pushFront(data_t &vec, num_t value, size_t count = 1)
{
	vec.insert(vec.begin(), count, value);
	return exp_t(count);
}
static inline void pushBack(data_t &vec, num_t value, size_t count = 1) { vec.insert(vec.end(), count, value); }


// Removes all trailing and leading zeros from the vector and returns the new exponent
static exp_t truncate(exp_t exp, data_t &vec)
{
	const auto
			front = std::find_if(vec.begin(), vec.end(), [](const auto &value) { return value; }),
			back = std::find_if(vec.rbegin(), vec.rend(), [](const auto &value) { return value; }).base();

	// Vector is full of zeros
	if(back <= front) {
		vec.clear();

		return 0;
	}
	else {
		// Change in exponent is the number of leading zeros
		const exp_t expChange = std::distance(vec.begin(), front);

		// Move non-zero values to front and erase the rest
		std::rotate(vec.begin(), front, back);
		vec.resize(size_t(std::distance(front, back)));

		return exp - expChange;
	}
}


// Turn an overflown two's-complement vector into a positive one
static inline Sign turnNegative(data_t &vec) noexcept
{
	rneg(rptr(vec), vec.size());
	return Sign::Negative;
}


// Adds two vectors into result, and returns the final exponent
static exp_t add(data_t &result,
				 exp_t leftExp, data_t &&left,
				 exp_t rightExp, data_t &&right)
{
	const exp_t
			leftMinExp = minExp(leftExp, left),
			rightMinExp = minExp(rightExp, right);

	// Determine correct boundaries
	const bool
			leftIsUpper = leftExp > rightExp,
			leftIsLower = leftMinExp < rightMinExp;

	data_t
			&notUpper = leftIsUpper ? right : left,
			&notLower = leftIsLower ? right : left;

	const exp_t
			upperExp = leftIsUpper ? leftExp : rightExp,
			notUpperExp = leftIsUpper ? rightExp : leftExp,
			lowerMinExp = leftIsLower ? leftMinExp : rightMinExp,
			notLowerMinExp = leftIsLower ? rightMinExp : leftMinExp;

	// Prepare result
	const auto size = size_t(upperExp - lowerMinExp);
	result.clear();
	result.resize(size);

	// Equalize number sizes
	pushFront(notUpper, 0, size_t(upperExp - notUpperExp));
	pushBack(notLower, 0, size_t(notLowerMinExp - lowerMinExp));

	exp_t expDelta = 0;

	// Sum and account for overflow
	if(radd(rptr(result), rptr(left), rptr(right), size))
		expDelta = pushFront(result, 1);

	return truncate(upperExp + expDelta, result);
}


// Subtracts two vectors into result, and returns the final exponent and sign
struct SubResult {
	exp_t exp;
	Sign sign;
};

static SubResult sub(data_t &result,
					 exp_t leftExp, data_t &&left,
					 exp_t rightExp, data_t &&right)
{
	const exp_t
			leftMinExp = minExp(leftExp, left),
			rightMinExp = minExp(rightExp, right);

	// Determine correct boundaries
	const bool
			leftIsUpper = leftExp > rightExp,
			leftIsLower = leftMinExp < rightMinExp;

	data_t
			&notUpper = leftIsUpper ? right : left,
			&notLower = leftIsLower ? right : left;

	const exp_t
			upperExp = leftIsUpper ? leftExp : rightExp,
			notUpperExp = leftIsUpper ? rightExp : leftExp,
			lowerMinExp = leftIsLower ? leftMinExp : rightMinExp,
			notLowerMinExp = leftIsLower ? rightMinExp : leftMinExp;

	// Prepare result
	const auto size = size_t(upperExp - lowerMinExp);
	result.clear();
	result.resize(size);

	// Equalize number sizes
	pushFront(notUpper, 0, size_t(upperExp - notUpperExp));
	pushBack(notLower, 0, size_t(notLowerMinExp - lowerMinExp));

	// Sub and account for negative result on overflow
	Sign sign = Sign::Positive;

	if(rsub(rptr(result), rptr(left), rptr(right), size))
		sign = turnNegative(result);

	return {
			truncate(upperExp, result),
			sign
	};
}


// Multiplies two vectors into result, and returns the final exponent
static exp_t multiply(data_t &result,
					  exp_t leftExp, const data_t &left,
					  exp_t rightExp, const data_t &right)
{
	const bool
			leftIsBigger = left.size() > right.size();

	const data_t
			&bigger = leftIsBigger ? left : right,
			&smaller = leftIsBigger ? right : left;

	const size_t
			sizeChange = smaller.size() + 1,
			size = bigger.size() + sizeChange;

	result.clear();
	result.resize(size);

	rmul(rptr(result), rptr(bigger), bigger.size(), rptr(smaller), smaller.size());

	return truncate(leftExp + rightExp + exp_t(sizeChange), result);
}


// Squares a vector into result, and returns the final exponent
static exp_t square(data_t &result,
					exp_t numExp, const data_t &num)
{
	const size_t
			numSize = num.size(),
			sizeChange = numSize + 1,
			size = numSize + sizeChange;

	result.clear();
	result.resize(size, 0);

	rmul(rptr(result), rptr(num), numSize, rptr(num), numSize);

	return truncate(numExp + numExp + exp_t(sizeChange), result);
}


// Computes a vector to power exp into result, and returns the final exponent
static exp_t power(data_t &result, exp_t numExp, const data_t &num, uexp_t exp)
{
	exp_t resultExp = 0;

	// Intermediate square computation is double buffered between buffer[0] <-> buffer[1]
	// Final result computation is double buffered between        buffer[2] <-> result
	data_t
			buffer[3] = {num, {}, {1}},

			*oldValue = buffer,
			*newValue = buffer + 1,

			*oldResult = buffer + 2,
			*newResult = &result;

	do {
		// If the current power has the bit set, multiply the result by the current power value
		// oldResult always keeps the current result, as they are swapped after every multiplication
		if(exp & 1u) {
			resultExp = multiply(*newResult, resultExp, *oldResult, numExp, *oldValue);
			std::swap(oldResult, newResult);
		}

		// Square the current power value
		// oldValue always keeps the current power value, as they are swapped after every multiplication
		numExp = square(*newValue, numExp, *oldValue);
		std::swap(oldValue, newValue);

		exp >>= 1u;
	} while(exp);

	// If the result of the computation is not in the output vector, move it there
	if(oldResult != &result)
		result = std::move(*oldResult);

	return resultExp;
}



//-NUMBER-ARITHMETIC-PRELIMINARY-CHECKS--------------------------------------------------------------------------------
//    These functions Are run before the actual computation to do bound checking, and return true if they pass
//    If they return false, they MUST set the result and that value will be returned

static bool checkAdd(number &result, const number &left, const number &right)
{
	const bool
			leftUndef = left.isUndefined(),
			rightUndef = right.isUndefined(),
			leftNan = left.isNaN(),
			rightNan = right.isNaN(),
			leftZero = left.isZero(),
			rightZero = right.isZero();

	if(leftUndef | rightUndef)
		result = number::Undefined();
	else if(leftZero)
		result = right;
	else if(rightZero)
		result = left;
	else if(leftNan | rightNan)
		result = number::NaN();
	else
		return true;
	return false;
}

static bool checkSub(number &result, const number &left, const number &right)
{
	const bool
			leftUndef = left.isUndefined(),
			rightUndef = right.isUndefined(),
			leftNan = left.isNaN(),
			rightNan = right.isNaN(),
			leftZero = left.isZero(),
			rightZero = right.isZero();

	if(leftUndef | rightUndef)
		result = number::Undefined();
	else if(leftZero)
		result = -right;
	else if(rightZero)
		result = left;
	else if(leftNan | rightNan)
		result = number::NaN();
	else
		return true;
	return false;
}

static bool checkMultiply(number &result, const number &left, const number &right)
{
	const bool
			leftUndef = left.isUndefined(),
			rightUndef = right.isUndefined(),
			leftNan = left.isNaN(),
			rightNan = right.isNaN(),
			leftZero = left.isZero(),
			rightZero = right.isZero();

	if(leftUndef | rightUndef | (leftNan & rightZero) | (rightNan & leftZero))
		result = number::Undefined();
	else if(leftZero | rightZero)
		result = number::Zero();
	else if(leftNan | rightNan)
		result = number::NaN();
	else
		return true;
	return false;
}

static bool checkDivide(number &result, const number &left, const number &right)
{
	const bool
			leftUndef = left.isUndefined(),
			rightUndef = right.isUndefined(),
			leftNan = left.isNaN(),
			rightNan = right.isNaN(),
			leftZero = left.isZero(),
			rightZero = right.isZero();

	if(leftUndef | rightUndef | (leftNan & rightNan) | (leftZero & rightZero))
		result = number::Undefined();
	else if(leftZero | rightNan)
		result = number::Zero();
	else if(leftNan | rightZero)
		result = number::NaN();
	else
		return true;
	return false;
}

static bool checkPower(number &result, const number &num, exp_t exp)
{
	const bool
			undef = num.isUndefined(),
			nan = num.isNaN(),
			zero = num.isZero();

	if(undef | zero | nan)
		result = num;
	else if(!exp)
		result = number::One();
	else
		return true;
	return false;
}

static bool checkSqrt(number &result, const number &num)
{
	const bool
			undef = num.isUndefined(),
			nan = num.isNaN(),
			zero = num.isZero();

	if(undef)
		result = number::Undefined();
	else if(zero)
		result = number::Zero();
	else if(nan)
		result = number::NaN();
	else
		return true;
	return false;
}



//-CONSTRUCTORS--------------------------------------------------------------------------------------------------------

number::number(int value) :
		m_nom{num_t(std::abs(value))},
		m_den{1},
		m_sign{value >= 0}
{
	m_nomExp = truncate(m_nomExp, m_nom);
}



//-OPERATORS-----------------------------------------------------------------------------------------------------------

number number::operator-() const
{
	number result = *this;
	result.negate();
	return result;
}

number operator+(const number &left, const number &right)
{
	switch(left.sign()) {
		case Sign::Positive:
			switch(right.sign()) {
				// +left + +right <=> left + right
				case Sign::Positive:
					return number::AddPositive(left, right);

					// +left + -right <=> left - right
				case Sign::Negative:
					return number::SubPositive(left, right);
			}

		case Sign::Negative:
			switch(right.sign()) {
				// -left + +right <=> right - left
				case Sign::Positive:
					return number::SubPositive(right, left);

					// -left + -right <=> -(left + right)
				case Sign::Negative:
					return number::AddPositive(left, right).negate();
			}
	}

	// Remove compiler warning
	return {};
}

number operator-(const number &left, const number &right)
{
	switch(left.sign()) {
		case Sign::Positive:
			switch(right.sign()) {
				// +left - +right <=> left - right
				case Sign::Positive:
					return number::SubPositive(left, right);

					// +left - -right <=> left + right
				case Sign::Negative:
					return number::AddPositive(left, right);
			}

		case Sign::Negative:
			switch(right.sign()) {
				// -left - +right <=> -(left + right)
				case Sign::Positive:
					return number::AddPositive(right, left).negate();

					// -left - -right <=> right - left
				case Sign::Negative:
					return number::SubPositive(right, left).negate();
			}
	}

	// Remove compiler warning
	return {};
}

number operator*(const number &left, const number &right)
{
	return number::Multiply(left, right);
}

number operator/(const number &left, const number &right)
{
	return number::Divide(left, right);
}



//-ARITHMETIC-MEMBER-FUNCTIONS-----------------------------------------------------------------------------------------

number number::power(exp_t exp) const
{
	return Power(*this, exp);
}

number number::sqrt(digits_t) const
{
	// TODO
	return {};
}



//-STATIC-ARITHMETIC-HELPER-METHODS------------------------------------------------------------------------------------

number number::AddPositive(const number &left, const number &right)
{
	number result;

	if(checkAdd(result, left, right)) {
		data_t leftNormal, rightNormal;

		const exp_t
				leftExp = multiply(leftNormal, left.m_nomExp, left.m_nom, right.m_denExp, right.m_den),
				rightExp = multiply(rightNormal, right.m_nomExp, right.m_nom, left.m_denExp, left.m_den);

		result.m_denExp = multiply(result.m_den, left.m_denExp, left.m_den, right.m_denExp, right.m_den);
		result.m_nomExp = add(result.m_nom, leftExp, std::move(leftNormal), rightExp, std::move(rightNormal));
	}

	return result;
}

number number::SubPositive(const number &left, const number &right)
{
	number result;

	if(checkSub(result, left, right)) {
		data_t leftNormal, rightNormal;

		const exp_t
				leftExp = multiply(leftNormal, left.m_nomExp, left.m_nom, right.m_denExp, right.m_den),
				rightExp = multiply(rightNormal, right.m_nomExp, right.m_nom, left.m_denExp, left.m_den);

		result.m_denExp = multiply(result.m_den, left.m_denExp, left.m_den, right.m_denExp, right.m_den);

		const SubResult subResult = sub(result.m_nom, leftExp, std::move(leftNormal), rightExp, std::move(rightNormal));
		result.m_nomExp = subResult.exp;
		result.m_sign = subResult.sign;
	}

	return result;
}

number number::Multiply(const number &left, const number &right)
{
	number result;

	if(checkMultiply(result, left, right)) {
		result.m_nomExp = multiply(result.m_nom, left.m_nomExp, left.m_nom, right.m_nomExp, right.m_nom);
		result.m_denExp = multiply(result.m_den, left.m_denExp, left.m_den, right.m_denExp, right.m_den);

		result.m_sign = left.m_sign ^ right.m_sign;
	}

	return result;
}

number number::Divide(const number &left, const number &right)
{
	number result;

	if(checkDivide(result, left, right)) {
		result.m_nomExp = multiply(result.m_nom, left.m_nomExp, left.m_nom, right.m_denExp, right.m_den);
		result.m_denExp = multiply(result.m_den, left.m_denExp, left.m_den, right.m_nomExp, right.m_nom);

		result.m_sign = left.m_sign ^ right.m_sign;
	}

	return result;
}

number number::Power(const number &num, exp_t exp)
{
	number result;

	if(checkPower(result, num, exp)) {
		if(exp > 0) {
			const auto uexp = uexp_t(exp);

			result.m_nomExp = ::power(result.m_nom, num.m_nomExp, num.m_nom, uexp);
			result.m_denExp = ::power(result.m_den, num.m_denExp, num.m_den, uexp);
		}
		else {
			const auto uexp = uexp_t(-exp);

			result.m_nomExp = ::power(result.m_nom, num.m_denExp, num.m_den, uexp);
			result.m_denExp = ::power(result.m_den, num.m_nomExp, num.m_nom, uexp);
		}
	}

	return result;
}

number number::Sqrt(const number &num, digits_t)
{
	number result;

	if(checkSqrt(result, num)) {

	}

	return result;
}



//-GLOBAL-OPERATOR-OVERLOADS-------------------------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &out, const number &value)
{
	const auto printVec = [&out](const char *name, const data_t &vec, exp_t exp) {
		out
				<< "  " << name << "Exp: " << exp << "\n"
				<< "  " << name << ": [ " << std::hex;

		for(const auto &val : vec)
			out << val << " ";

		out << "]\n"
			<< "        0x";

		for(const auto &val : vec)
			out << std::setfill('0') << std::setw(8) << val;

		out << std::dec << "\n";
	};

	out << "{\n"
		<< "  sign: " << (value.sign() ? '+' : '-') << "\n"
		<< "  exp : " << value.exp() << "\n";

	printVec("nom", value.nom(), value.nomExp());
	printVec("den", value.den(), value.denExp());

	return out << "}\n";
}

int main()
{
	//number a(0x7fffffff), b(0x7fffffff), apb = a + b, apbpapb = apb + apb, apbpapbpa = apbpapb + a;
	number
			a(Sign::Positive, 2, data_t{2}, 0, data_t{3}),
			b(Sign::Positive, 0, data_t{3}, 0, data_t{1});
	//		a(2, 5, number::data_t({ 0xffffffff, 0xffffffff })),
	//		b(2, 2, number::data_t({ 0xffffffff, 0xffffffff }));

	std::cout
			<< "a: " << a << "\n"
			<< "b: " << b << "\n"

			<< "a * b: " << a.power(4) << "\n";

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

