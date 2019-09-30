// return in mod t
template
<class T>
inline T modulo(T in, T t) {
	T rem = in % t;
	if (rem < 0) return t + rem;
	else return rem;
}

template
<class T>
T gcd(T a, T b)
{
	if (b == 0) return a;
	else return gcd<T>(b, a % b);
}

template
<class T>
T lcm(T a, T b)
{
	return a * b / gcd<T>(a, b);
}

template <class T>
struct Fraction
{
    T Num;
    T Den;

    Fraction()
    {
        Num = Den = 1;
    }

    template <class K>
    Fraction(K num, K den)
    {
        Num = num;
        Den = den;
    }

    void fromDouble(double in)
    {
        double d = 0;
        Num = 0;
        Den = 1;
        while (d != in)
        {
            if (d < in)	++Num;
            else if (d > in) ++Den;
            d = static_cast<double>(Num) / Den;
        }
    }

	Fraction<T> Simplify() {
		T t = gcd(Num, Den);
		return Fraction{ Num / t, Den / t };
	}

	operator double() {
		return Num / Den;
	}

	bool operator<(Fraction<T> other) {
		return this->operator double() < other->operator double();
	}
};

using LFraction = Fraction<long long>;
using IFraction = Fraction<int>;


template <class T>
T abs(T x)
{
    return x > 0 ? x : -x;
}

inline bool IntervalsIntersect(const double a, const double b, const double c, const double d)
{
    return a <= d && c <= b;
}

template <class T>
inline T LerpRatio(const T &Start, const T& End, double Progress, double Total)
{
    return Start + (End - Start) * Progress / Total;
}

template <class T, class N>
inline T Lerp(const T &Start, const T& End, N k)
{
    return Start + k * (End - Start);
}

template <class T>
inline T Clamp(const T &Value, const T &Min, const T &Max)
{
    if (Value < Min) return Min;
    else if (Value > Max) return Max;
    else return Value;
}

template <class T>
inline T clamp_to_interval(const T& value, const T& target, const T& interval)
{
    T output = value;
    while (output > target + interval) output -= interval * 2;
    while (output < target - interval) output += interval * 2;
    return output;
}

template <class T>
T sign(T num) {
	return (num > T(0)) - (num < T(0));
}

int LCM(const std::vector<int> &Set);
double latof(std::string s);

