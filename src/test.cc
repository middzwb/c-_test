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
        :out_(move(r)) //
    {
        cout << "move rr constructor" << endl;
    }
    Response(shared_ptr<T>& r)
        :out_(move(r))
    {cout << "move lr constructor" << endl;}
    const T& out() const { cout << "const" << endl; return *out_; }
    T& out() { cout << "no const" << endl; return *out_; }
public:
    shared_ptr<T> out_;
};

void test_response() {
    class Temp {
    public:
        Temp() = default;
        Temp(string a) // if param is rvalue reference, will call move constructor
            :a_(move(a))
        { cout << "non rvalue reference constructor" << endl; }
        Temp(Temp&& other)
            :a_(move(other.a_))
        {cout << "move constructor" << endl;}
        Temp(const Temp& other)
            :a_(other.a_)
        {cout << "copy constructor" << endl; }
        //Temp(string&& a)
        //    :a_(move(a))
        //{ cout << "rvalue reference" << endl; }
        ~Temp() { cout << "~Temp" << endl; }
        Temp& operator=(Temp&& other) {
            cout << "move assignment operator" << endl;
            a_ = std::move(other.a_);
            return *this;
        }
        Temp& operator=(const Temp& other) {
            cout << "copy assignment operator" << endl;
            a_ = other.a_;
            return *this;
        }
        operator bool() const { return false; }
        const string& v() const { return a_; }
    private:
        string a_;
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

        auto s2{make_shared<ostringstream>()};
        assert(s2.use_count() == 1);
        Response<ostringstream> r2(s2);
        assert(s2.use_count() == 0 && r2.out_.use_count() == 1);
    }

    {
        cout << "Test optional ..." << endl;
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
        header.emplace(unordered_map<string, string>({{"foo", "bar"}}));
        assert(header->size() == 1 && header->at("foo") == "bar");
        auto str2{make_optional<string>()}; // construct string, string is empty; not nullopt
        assert(str2);
        optional<string> str;
        assert(!str);
    }
    {
        cout << "Test move ..." << endl;
        // param is obj, argument is obj
        class TestMove {
        public:
            TestMove(Temp t)
                :t_(move(t))
            {}

            Temp t_;
        };
        Temp t;
        TestMove tm(move(t));
        Temp t1;
        Temp t2{Temp()}; // why not copy constructor?
        // Temp t2(Temp()); // ‘test_response()::Temp t2(test_response()::Temp (*)())’, declared using local type ‘test_response()::Temp’, is used but never defined
    }
    {
        cout << "Test move assignment ..." << endl;
        Temp t1("a");
        Temp t2;
        t2 = t1;
        assert(t1.v() == "a");
        Temp t3;
        t3 = move(t1);
        assert(t3.v() == "a");
    }
}

void test_future() {
    cout << "Test future ..." << endl;
    promise<void> pro;
    auto fu = pro.get_future();
    unordered_map<string, string> tmp;

    using namespace std::chrono_literals;
    thread worker{[&pro, &tmp]() {
        this_thread::sleep_for(1s);
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

void test_stream() {
    cout << "Test stream ..." << endl;
    istringstream istr;
    assert(istr);
    istr.str("hello");
    const size_t size = 8192;
    char buf[size];
    istr.read(buf, size);
    auto count = istr.gcount();
    while (count > 0) {
        istr.read(buf, size);
        count = istr.gcount();
    }
}

void main_test() {
    test_make_request();
    test_response();
    test_future();
    test_stream();
}

int main()
{
    main_test();
}
