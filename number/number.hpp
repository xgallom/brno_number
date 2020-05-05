#include <cstdint>
#include <cstdlib>
#include <iostream>
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
	using exp_t = int32_t;

	// A type for a vector of numeric chunks
	using data_t = std::vector<num_t>;


	//-CONSTANT-DEFINITIONS--------------------------------------------------------------------------------------------

	static constexpr exp_t DefaultExponent = 0;

	// Bit offset of the overflow part of the result
	static constexpr result_t OverflowOffset = std::numeric_limits<result_t>::digits >> 1;
	// Bit mask of the result part of the result
	static constexpr result_t ResultMask = std::numeric_limits<result_t>::max() >> OverflowOffset;
	// Bit mask of the overflow part of the result
	static constexpr result_t OverflowMask = ~ResultMask;


	//-MEMBER-DECLARATIONS---------------------------------------------------------------------------------------------
private:
	// Value = (m_sign ? 1 : -1) * (m_nom / m_den) * 2^(sizeof(chunk) * m_exp)

	// Nominator and Denominator
	data_t m_nom = {};
	data_t m_den = {};

	// Exponent
	exp_t m_exp = DefaultExponent;

	// Sign
	sign_t m_sign = Positive;


	//-DEFAULT-CONSTRUCTORS-&-ASSIGNMENTS------------------------------------------------------------------------------
public:
	number() = default;
	number(const number&) = default;
	number(number&&) = default;
	number& operator=(const number&) = default;
	number& operator=(number&&) = default;


	//-CONSTRUCTORS----------------------------------------------------------------------------------------------------

	// Implicitly convertible constructor from an int value
	number(int value);

	// Explicit constructor for testing and debugging purposes
	inline explicit number(Sign sign, exp_t exp = DefaultExponent, data_t&& nom = {}, data_t&& den = {})
		: m_nom{ std::move(nom) }, m_den{ std::move(den) }, m_exp{ exp }, m_sign{ sign }
	{}


	//-MEMBER-ACCESSORS------------------------------------------------------------------------------------------------

	inline exp_t exp() const { return m_exp; }
	inline Sign sign() const { return static_cast<Sign>(m_sign); }
	inline const data_t& nom() const { return m_nom; }
	inline const data_t& den() const { return m_den; }
	inline bool isZero() const { return m_nom.empty() || m_den.empty(); }


	//-OPERATORS-------------------------------------------------------------------------------------------------------

	number operator-() const;

	friend number operator+(number left, number right);
	friend number operator-(number left, number right);
	friend number operator*(const number& left, const number& right);
	friend number operator/(const number& left, const number& right);

	number power(exp_t exp) const;
	number sqrt(digits_t digits) const;

	// Returns true if non-zero
	explicit operator bool() const;


	//-INTERNAL-HELPER-METHODS-----------------------------------------------------------------------------------------
protected:
	// Constructor for empty initialization of specified size
	inline number(Sign sign, exp_t exp, size_t nomSize, size_t denSize)
		: m_nom(nomSize, {}), m_den(denSize, {}), m_exp{ exp }, m_sign{ sign }
	{}


	//-STATIC-ARITHMETIC-HELPER-METHODS--------------------------------------------------------------------------------
public:
	static number AddPositive(number&& left, number&& right);
	static number SubPositive(number&& left, number&& right);
	static number Multiply(const number& left, const number& right);
	static number Divide(const number& left, const number& right);
	static number Power(const number& num, exp_t exp);
	static number Sqrt(const number& num, digits_t digits);
};

// Output stream operator overload for debug purposes
std::ostream& operator<<(std::ostream& out, const number& value);
