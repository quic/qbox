// This test case was reduced from
// smmu500/include/quic/smmu500/smmu500.h, it demonstrates a lambda
// capture that older releases of gcc will incorrectly reject.

template <bool>
using a = int;
template <typename>
class c;
template <typename aa, typename... d>
class c<aa(d...)>
{
    template <typename, typename>
    using e = int;

public:
    template <typename f, typename = e<a<!bool()>, void>,
              typename = e<int, void>>
    c(f);
};
template <typename aa, typename... d>
template <typename f, typename, typename>
c<aa(d...)>::c(f) {
}
class g
{
public:
    g(char*);
};
template <int = 2>
class s
{
    void z();
    c<void()> f = [&] { z(); };

public:
    s(g) {}
};
class ag
{
    s<> as;
    ag(): as("") {}
};
int main() {
}
