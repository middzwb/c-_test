#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <cassert>

using namespace std;

shared_ptr<ostringstream> make_request(const shared_ptr<istringstream>& ss) {
    if (ss) {
        auto s = ss->str();
        auto ret{make_shared<ostringstream>()};
        ret->write(s.data(), s.size());
        return ret;
    }
    return nullptr;
}
void test_make_request() {
    auto ss{make_shared<istringstream>("test sstringstream")};
    auto ret = make_request(ss);
    if (ret) {
        cout << ret->str() << endl;
    }
    auto ret2 = make_request(nullptr);
    assert(!ret2);
}

template<typename T>
class Response {
public:
    Response()
        :out_(make_shared<T>())
    {}
    ~Response() = default;
    Response(shared_ptr<T>&& r)
        :out_(move(r))
    {
        cout << "move constructor" << endl;
    }
    const T& out() const { cout << "const" << endl; return *out_; }
    T& out() { cout << "no const" << endl; return *out_; }
public:
    shared_ptr<T> out_;
};
void test_response() {
    class Temp {
    public:
        Temp() = default;
        ~Temp() { cout << "temp destructor" << endl; }
    };
    {
        cout << "Test move ..." << endl;
        Response<ostringstream> resp;
        resp.out().str("hello");
        cout << resp.out().str() << endl;
    }

    {
        auto s{make_shared<ostringstream>()};
        s->str("move");
        assert(s.use_count() == 1);
        Response<ostringstream> mr(move(s));
        assert(s.use_count() == 0 && mr.out_.use_count() == 1);
    }

}

void main_test() {
    test_make_request();
    test_response();
}

int main()
{
    main_test();
}
