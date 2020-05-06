#include "number.hpp"

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

int main()
{
	//number a(0x7fffffff), b(0x7fffffff), apb = a + b, apbpapb = apb + apb, apbpapbpa = apbpapb + a;
	number
			a(Sign::Negative, 2, data_t{2, 1}, 0, data_t{4}),
			b(Sign::Negative, 2, data_t{4}, 0, data_t{8});
	//		a(2, 5, number::data_t({ 0xffffffff, 0xffffffff })),
	//		b(2, 2, number::data_t({ 0xffffffff, 0xffffffff }));

	std::cout
			<< "a: " << a << "\n"
			<< "b: " << b << "\n"

			<< "a == a: " << (a == a) << "\n"
			<< "a == b: " << (a == b) << "\n"
			<< "a != a: " << (a != a) << "\n"
			<< "a != b: " << (a != b) << "\n"
			<< "a <  a: " << (a <  a) << "\n"
			<< "a <  b: " << (a <  b) << "\n"
			<< "a <= a: " << (a <= a) << "\n"
			<< "a <= b: " << (a <= b) << "\n"
			<< "a >  a: " << (a >  a) << "\n"
			<< "a >  b: " << (a >  b) << "\n"
			<< "a >= a: " << (a >= a) << "\n"
			<< "a >= b: " << (a >= b) << "\n";

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

