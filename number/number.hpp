#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>

class number {

	//-TYPE-DEFINITIONS------------------------------------------------------------------------------------------------
public:

	// Sign of the number
	using sign_t = bool;

	// Possible enumerations of the sign
	enum Sign : sign_t {
		Positive = true,
		Negative = false
	};

	// A type for a numeric chunk
	using num_t = uint32_t;
	using snum_t = int32_t;

	// A type for an arithmetic operation on a chunk (for overflow checking)
	using result_t = uint64_t;
	using sresult_t = int64_t;

	// Types for various numeric values
	using digits_t = uint32_t;
	using exp_t = int64_t;
	using uexp_t = uint64_t;

	// A type for a vector of numeric chunks
	using data_t = std::vector<num_t>;



	//-CONSTANT-DEFINITIONS--------------------------------------------------------------------------------------------

	static constexpr Sign DefaultSign = Positive;
	static constexpr exp_t DefaultExponent = 0;

	// Bit offset of the overflow part of the result
	static constexpr result_t OverflowOffset = std::numeric_limits<result_t>::digits / 2;
	// Bit mask of the result part of the result
	static constexpr result_t ResultMask = std::numeric_limits<result_t>::max() >> OverflowOffset;
	// Bit mask of the overflow part of the result
	static constexpr result_t OverflowMask = ~ResultMask;



	//-MEMBER-DECLARATIONS---------------------------------------------------------------------------------------------
	// Value = (m_sign ? 1 : -1) * (m_nom / m_den) * 2^(sizeof(chunk) * m_exp)
private:

	// Nominator and Denominator
	data_t m_nom = {};
	data_t m_den = {};

	// Exponent
	exp_t m_nomExp = DefaultExponent;
	exp_t m_denExp = DefaultExponent;

	// Sign
	sign_t m_sign = DefaultSign;



	//-DEFAULT-CONSTRUCTORS-&-ASSIGNMENTS------------------------------------------------------------------------------
public:
	number() = default;
	number(const number &) = default;
	number(number &&) noexcept = default;
	number &operator=(const number &) = default;
	number &operator=(number &&) noexcept = default;



	//-CONSTRUCTORS----------------------------------------------------------------------------------------------------

	// Implicitly convertible constructor from an int value
	number(int value);

	// Explicit constructor for testing and debugging purposes
	inline explicit number(Sign sign, exp_t nomExp, data_t &&nom, exp_t denExp, data_t &&den) noexcept :
			m_nom{std::move(nom)},
			m_den{std::move(den)},
			m_nomExp{nomExp},
			m_denExp{denExp},
			m_sign{sign} {}


	// Special value constructors
	static inline number Zero()
	{
		return number(Positive, DefaultExponent, data_t{}, DefaultExponent, data_t{1});
	}
	static inline number NaN()
	{
		return number(Positive, DefaultExponent, data_t{1}, DefaultExponent, data_t{});
	}
	static inline number Undefined()
	{
		return number();
	}
	static inline number One()
	{
		return number(Positive, DefaultExponent, data_t{1}, DefaultExponent, data_t{1});
	}



	//-MEMBER-ACCESSORS------------------------------------------------------------------------------------------------

	inline const data_t &nom() const noexcept { return m_nom; }
	inline const data_t &den() const noexcept { return m_den; }
	inline exp_t exp() const noexcept { return m_nomExp - m_denExp; }
	inline exp_t nomExp() const noexcept { return m_nomExp; }
	inline exp_t denExp() const noexcept { return m_denExp; }
	inline Sign sign() const noexcept { return static_cast<Sign>(m_sign); }

	inline bool isZero() const noexcept { return m_nom.empty() & !m_den.empty(); }
	inline bool isNonZero() const noexcept { return !m_nom.empty(); }
	inline bool isNaN() const noexcept { return !m_nom.empty() & m_den.empty(); }
	inline bool isNotNaN() const noexcept { return !m_den.empty(); }
	inline bool isUndefined() const noexcept { return m_nom.empty() & m_den.empty(); }



	//-OPERATORS-------------------------------------------------------------------------------------------------------

	number operator-() const;

	// Returns true if number is non-zero
	explicit inline operator bool() const noexcept { return isNonZero(); }



	//-ARITHMETIC-MEMBER-FUNCTIONS-------------------------------------------------------------------------------------

	// Turn a number negative
	inline number &negate() noexcept
	{
		m_sign = !m_sign;
		return *this;
	}

	number power(exp_t exp) const;
	number sqrt(digits_t digits) const;



	//-INTERNAL-HELPER-METHODS-----------------------------------------------------------------------------------------
protected:

	// Constructor for empty initialization of specified size
	inline number(Sign sign, exp_t nomExp, size_t nomSize, exp_t denExp, size_t denSize) :
			m_nom(nomSize, {}),
			m_den(denSize, {}),
			m_nomExp{nomExp},
			m_denExp{denExp},
			m_sign{sign} {}



	//-STATIC-ARITHMETIC-HELPER-METHODS--------------------------------------------------------------------------------
public:
	static number AddPositive(const number &left, const number &right);
	static number SubPositive(const number &left, const number &right);
	static number Multiply(const number &left, const number &right);
	static number Divide(const number &left, const number &right);
	static number Power(const number &num, exp_t exp);
	static number Sqrt(const number &num, digits_t digits);

	static bool Equal(const number &left, const number &right);
	static bool NotEqual(const number &left, const number &right);
	static bool Less(const number &left, const number &right);
	static bool LessEqual(const number &left, const number &right);
	static bool More(const number &left, const number &right);
	static bool MoreEqual(const number &left, const number &right);
};



//-GLOBAL-OPERATOR-OVERLOADS-------------------------------------------------------------------------------------------

number operator+(const number &left, const number &right);
number operator-(const number &left, const number &right);
number operator*(const number &left, const number &right);
number operator/(const number &left, const number &right);

bool operator==(const number &left, const number &right);
bool operator!=(const number &left, const number &right);
bool operator<(const number &left, const number &right);
bool operator<=(const number &left, const number &right);
bool operator>(const number &left, const number &right);
bool operator>=(const number &left, const number &right);

std::ostream &operator<<(std::ostream &out, const number &value);
