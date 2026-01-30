#include <getopt.h>
#include <mathlib.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum class operation { none,
    add,
    sub,
    mul,
    div,
    pow,
    fact };

enum class exit_code : int {
    ok = 0,
    usage = 1,
    math = 2,
};

namespace {

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

void print_help(const char* prog)
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

bool parse_int(const char* s, int* out)
{
    if (!s || !out)
        return false;

    char* end = nullptr;
    long v = std::strtol(s, &end, 10);

    if (end == s || *end != '\0')
        return false;
    if (v < INT_MIN || v > INT_MAX)
        return false;

    *out = static_cast<int>(v);
    return true;
}

bool parse_operation(const char* s, operation* out)
{
    if (!s || !out)
        return false;

    for (size_t i = 0; i < kOpsCount; ++i) {
        if (std::strcmp(s, kOps[i].name) == 0) {
            *out = kOps[i].op;
            return true;
        }
    }
    return false;
}

bool needs_b(operation op)
{
    switch (op) {
    case operation::add: {
        return true;
    }
    case operation::sub: {
        return true;
    }
    case operation::mul: {
        return true;
    }
    case operation::div: {
        return true;
    }
    case operation::pow: {
        return true;
    }
    case operation::fact: {
        return false;
    }
    case operation::none: {
        return false;
    }
    default: {
        return false;
    }
    }
}

int print_math_error(const char* name, mathlib::ml_error err)
{
    if (err == mathlib::ml_error::div0) {
        std::fprintf(stderr, "Error: %s: division by zero\n", name);
    } else {
        std::fprintf(stderr, "Error: %s: unknown math error\n", name);
    }
    return static_cast<int>(exit_code::math);
}

int run_operation(operation op, int a, int b)
{
    switch (op) {
    case operation::add: {
        auto r = mathlib::ml_add(a, b);
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::sub: {
        auto r = mathlib::ml_sub(a, b);
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::mul: {
        auto r = mathlib::ml_mul(a, b);
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::div: {
        auto r = mathlib::ml_div(a, b);
        if (r.error != mathlib::ml_error::ok)
            return print_math_error("div", r.error);
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::pow: {
        if (b < 0) {
            std::fprintf(stderr, "Error: pow: domain error (b must be >= 0)\n");
            return static_cast<int>(exit_code::math);
        }
        auto r = mathlib::ml_pow(a, static_cast<unsigned int>(b));
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::fact: {
        if (a < 0) {
            std::fprintf(stderr, "Error: fact: domain error (a must be >= 0)\n");
            return static_cast<int>(exit_code::math);
        }
        auto r = mathlib::ml_fact(static_cast<unsigned int>(a));
        std::printf("%d\n", r.value);
        return static_cast<int>(exit_code::ok);
    }
    case operation::none: {
        std::fprintf(stderr, "Error: unknown operation\n");
        return static_cast<int>(exit_code::usage);
    }
    default: {
        std::fprintf(stderr, "Error: unknown operation\n");
        return static_cast<int>(exit_code::usage);
    }
    }
}

} // namespace

int main(int argc, char** argv)
{
    int a = 0, b = 0;
    bool have_a = false, have_b = false;

    operation op = operation::none;
    bool have_op = false;

    static const option long_opts[] = {
        { "op", required_argument, nullptr, 'o' },
        { "a", required_argument, nullptr, 'a' },
        { "b", required_argument, nullptr, 'b' },
        { "help", no_argument, nullptr, 'h' },
        { nullptr, 0, nullptr, 0 },
    };

    opterr = 0;

    int c = 0;
    while ((c = getopt_long(argc, argv, "o:a:b:h", long_opts, nullptr)) != -1) {
        switch (c) {
        case 'o': {
            have_op = parse_operation(optarg, &op);
            if (!have_op) {
                std::fprintf(stderr, "Error: unknown operation '%s'\n", optarg);
                return static_cast<int>(exit_code::usage);
            }
            break;
        }
        case 'a': {
            have_a = parse_int(optarg, &a);
            if (!have_a) {
                std::fprintf(stderr, "Error: invalid integer for -a: '%s'\n", optarg);
                return static_cast<int>(exit_code::usage);
            }
            break;
        }
        case 'b': {
            have_b = parse_int(optarg, &b);
            if (!have_b) {
                std::fprintf(stderr, "Error: invalid integer for -b: '%s'\n", optarg);
                return static_cast<int>(exit_code::usage);
            }
            break;
        }
        case 'h': {
            print_help(argv[0]);
            return static_cast<int>(exit_code::ok);
        }
        default: {
            print_help(argv[0]);
            return static_cast<int>(exit_code::usage);
        }
        }
    }

    if (!have_op || !have_a) {
        std::fprintf(stderr, "Error: missing required arguments\n");
        print_help(argv[0]);
        return static_cast<int>(exit_code::usage);
    }

    if (!needs_b(op) && have_b) {
        std::fprintf(stderr, "Error: useless -b for this operation\n");
        print_help(argv[0]);
        return static_cast<int>(exit_code::usage);
    }

    if (needs_b(op) && !have_b) {
        std::fprintf(stderr, "Error: missing -b for this operation\n");
        print_help(argv[0]);
        return static_cast<int>(exit_code::usage);
    }

    if (op == operation::pow && b < 0) {
        std::fprintf(stderr, "Error: pow: domain error (b must be >= 0)\n");
        return static_cast<int>(exit_code::math);
    }

    return run_operation(op, a, b);
}
