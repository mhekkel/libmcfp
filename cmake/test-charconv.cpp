#include <charconv>
#include <cassert>
#include <cstring>

int main()
{
	float v;
	char s[] = "1.0";

	auto r = std::from_chars(s, s + strlen(s), v);

	assert(r.ec == std::errc{});
	assert(r.ptr = s + strlen(s));
	assert(v == 1.0f);

	return 0;
}