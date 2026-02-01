#include <cerrno>
#include <getopt.h>
#include <mathlib.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum class operation : std::uint8_t {
    none = 0,
    add,
    sub,
    mul,
    div,
    pow,
    fact
};

enum class exit_code : std::uint8_t {
    ok = 0,
    usage = 1,
    math = 2
};

namespace {

struct context {
    operation op = operation::none;
    bool have_op = false;

    std::int64_t a = 0;
    bool have_a = false;

    std::int64_t b = 0;
    bool have_b = false;

    mathlib::ml_result r {};
};

struct op_spec {
    const char* name;
    operation op;
};

constexpr op_spec kOps[] = {
    { "add", operation::add },
    { "sub", operation::sub },
    { "mul", operation::mul },
    { "div", operation::div },
    { "pow", operation::pow },
    { "fact", operation::fact },
};

constexpr size_t kOpsCount = sizeof(kOps) / sizeof(kOps[0]);

void help(const char* prog)
{
    std::printf(
        "Usage:\n"
        "  %s -o <op> -a <int> [-b <int>]\n"
        "\n"
        "Operations:\n"
        "  add   a + b\n"
        "  sub   a - b\n"
        "  mul   a * b\n"
        "  div   a / b   (checks division by 0)\n"
        "  pow   a ^ b   (b must be >= 0)\n"
        "  fact  a!      (a must be >= 0)\n"
        "\n"
        "Options:\n"
        "  -o, --op     operation name\n"
        "  -a, --a      first integer\n"
        "  -b, --b      second integer (required for add/sub/mul/div/pow)\n"
        "  -h, --help   show this help\n"
        "\n"
        "Examples:\n"
        "  %s -o add  -a 2  -b 3\n"
        "  %s -o fact -a 5\n",
        prog, prog, prog);
}

bool needs_b(operation op)
{
    return op == operation::add || op == operation::sub || op == operation::mul || op == operation::div || op == operation::pow;
}

bool parse_i64(const char* s, std::int64_t* out)
{
    if (!s || !out) {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    long long v = std::strtoll(s, &end, 10);

    if (end == s || *end != '\0') {
        return false;
    }
    if (errno == ERANGE) {
        return false;
    }

    *out = static_cast<std::int64_t>(v);
    return true;
}

bool parse_op(const char* s, operation* out)
{
    if (!s || !out) {
        return false;
    }
    for (size_t i = 0; i < kOpsCount; ++i) {
        if (std::strcmp(s, kOps[i].name) == 0) {
            *out = kOps[i].op;
            return true;
        }
    }
    return false;
}

exit_code print_math_err(const char* where, mathlib::ml_error e)
{
    if (e == mathlib::ml_error::div0) {
        std::fprintf(stderr, "Error: %s: division by zero\n", where);
    } else if (e == mathlib::ml_error::overflow) {
        std::fprintf(stderr, "Error: %s: overflow\n", where);
    } else {
        std::fprintf(stderr, "Error: %s: math error\n", where);
    }
    return exit_code::math;
}

exit_code print_result(const mathlib::ml_result& r)
{
    if (r.error != mathlib::ml_error::ok) {
        return print_math_err("calc", r.error);
    }

    if (r.kind == mathlib::ml_kind::i64) {
        std::printf("%lld\n", static_cast<long long>(r.value.i64));
    } else {
        std::printf("%llu\n", static_cast<unsigned long long>(r.value.u64));
    }

    return exit_code::ok;
}

exit_code parse(context& c, int argc, char** argv)
{
    const option long_opts[] = {
        { "op", required_argument, nullptr, 'o' },
        { "a", required_argument, nullptr, 'a' },
        { "b", required_argument, nullptr, 'b' },
        { "help", no_argument, nullptr, 'h' },
        { nullptr, 0, nullptr, 0 },
    };

    opterr = 0;
    int ch = 0;
    while ((ch = getopt_long(argc, argv, "o:a:b:h", long_opts, nullptr)) != -1) {
        switch (ch) {
        case 'o': {
            c.have_op = parse_op(optarg, &c.op);
            if (!c.have_op) {
                std::fprintf(stderr, "Error: unknown operation '%s'\n", optarg);
                return exit_code::usage;
            }
            break;
        }
        case 'a': {
            c.have_a = parse_i64(optarg, &c.a);
            if (!c.have_a) {
                std::fprintf(stderr, "Error: invalid integer for -a: '%s'\n", optarg);
                return exit_code::usage;
            }
            break;
        }
        case 'b': {
            c.have_b = parse_i64(optarg, &c.b);
            if (!c.have_b) {
                std::fprintf(stderr, "Error: invalid integer for -b: '%s'\n", optarg);
                return exit_code::usage;
            }
            break;
        }
        case 'h': {
            help(argv[0]);
            return exit_code::usage;
        }
        default: {
            help(argv[0]);
            return exit_code::usage;
        }
        }
    }
    return exit_code::ok;
}

exit_code check(const context& c, const char* prog)
{
    if (!c.have_op || !c.have_a) {
        std::fprintf(stderr, "Error: missing -o or -a\n");
        help(prog);
        return exit_code::usage;
    }
    if (!needs_b(c.op) && c.have_b) {
        std::fprintf(stderr, "Error: useless -b for this op\n");
        help(prog);
        return exit_code::usage;
    }
    if (needs_b(c.op) && !c.have_b) {
        std::fprintf(stderr, "Error: missing -b for this op\n");
        help(prog);
        return exit_code::usage;
    }
    if (c.op == operation::pow && c.b < 0) {
        std::fprintf(stderr, "Error: pow: domain error (b must be >= 0)\n");
        return exit_code::math;
    }
    if (c.op == operation::fact && c.a < 0) {
        std::fprintf(stderr, "Error: fact: domain error (a must be >= 0)\n");
        return exit_code::math;
    }
    return exit_code::ok;
}

exit_code calc(context& c)
{
    switch (c.op) {
    case operation::add: {
        c.r = mathlib::ml_add(c.a, c.b);
        break;
    }
    case operation::sub: {
        c.r = mathlib::ml_sub(c.a, c.b);
        break;
    }
    case operation::mul: {
        c.r = mathlib::ml_mul(c.a, c.b);
        break;
    }
    case operation::div: {
        c.r = mathlib::ml_div(c.a, c.b);
        break;
    }
    case operation::pow: {
        c.r = mathlib::ml_pow(c.a, static_cast<std::uint64_t>(c.b));
        break;
    }
    case operation::fact: {
        c.r = mathlib::ml_fact(static_cast<std::uint64_t>(c.a));
        break;
    }
    case operation::none:
    default: {
        std::fprintf(stderr, "Error: unknown operation\n");
        return exit_code::usage;
    }
    }
    return exit_code::ok;
}

int run(int argc, char** argv)
{
    context c {};
    exit_code rc = parse(c, argc, argv);
    if (rc != exit_code::ok) {
        return static_cast<int>(rc);
    }

    rc = check(c, argv[0]);
    if (rc != exit_code::ok) {
        return static_cast<int>(rc);
    }

    rc = calc(c);
    if (rc != exit_code::ok) {
        return static_cast<int>(rc);
    }
    return static_cast<int>(print_result(c.r));
}

} // namespace

int main(int argc, char** argv)
{
    return run(argc, argv);
}
