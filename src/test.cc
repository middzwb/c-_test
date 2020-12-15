#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <cassert>
#include <optional>
#include <map>
#include <unordered_map>
#include <future>
#include <thread>
#include <mutex>

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

        auto sstr{make_shared<stringstream>()};
        auto& ss = *sstr;
        ss << "a";
        ss << "b";
        assert(ss.str() == "ab");
    }

    {
        unordered_map<string, string> tmp;
        tmp["range"] = "0-1023";
        optional<unordered_map<string, string>> header(move(tmp));
        assert(tmp.empty());
        assert(header);
        assert((*header)["range"] == "0-1023");
        optional<unordered_map<string, string>> n;
        assert(!n);
        auto test_op = [](const optional<unordered_map<string, string>>& opt) -> bool {
            return opt.has_value();
        };
        assert(!test_op(nullopt));
    }
}

void test_future() {
    cout << "Test future ..." << endl;
    promise<void> pro;
    auto fu = pro.get_future();
    unordered_map<string, string> tmp;

    using namespace std::chrono_literals;
    thread worker{[&pro, &tmp]() {
        this_thread::sleep_for(3s);
        tmp.emplace("foo", "bar");
        pro.set_value();
    }};

    fu.get();
    assert(tmp.size() == 1 && tmp["foo"] == "bar");
    worker.join(); // https://en.cppreference.com/w/cpp/thread/thread/~thread; without join, terminate() is called.

    mutex m1;
    {
        lock_guard<mutex> locker(m1);
    }
    {
        lock_guard locker(m1);
    }
}

void main_test() {
    test_make_request();
    test_response();
    test_future();
}

int main()
{
    main_test();
}
