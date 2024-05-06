#include <reimu/core/error.h>
#include <reimu/core/result.h>

#include <assert.h>

struct Error1 : public reimu::ErrorBase {
public:
    std::string as_string() const override {
        return "Error1";
    };
};

struct Error2 : public reimu::ErrorBase {
public:
    char trash[1230];

    std::string as_string() const override {
        return "Error2";
    };
};

struct Error3 : public reimu::ErrorBase {
public:
    static int instance_count;
    char trash[1337];

    Error3() {
        instance_count++;
    }

    Error3(Error3&&) {
        instance_count++;
    }

    ~Error3() {
        instance_count--;
    }

    std::string as_string() const override {
        return "Error3";
    };
};

int Error3::instance_count = 0;

reimu::Result<int, reimu::ErrorBox> returns_an_error() {
    return ERR(Error3{});
}

int main() {
    {
        auto v = returns_an_error();

        assert(Error3::instance_count == 1);

        assert(v.is_err());
        assert(v.move_err().as_string().compare("Error3") == 0);
    }

    assert(Error3::instance_count == 0);

    return 0;
}
