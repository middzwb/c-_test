#include "MultiThread.h"

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
#include <type_traits>
#include <chrono>
#include <any>
#include <random>
#include <typeinfo>

using namespace std;

void test_reorder();
void template_test();
// x86-64 order: https://en.wikipedia.org/wiki/Memory_ordering#Runtime_memory_ordering
void test_seq_cst();
void test_acq_rel();

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
    auto ss{make_shared<istringstream>("Test sstringstream")};
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
        this_thread::sleep_for(1ms);
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
    {
        stringstream ss;
        ss << "hello" << " world";
        istringstream istr(move(ss.str()));
    }
}

class ZError {
public:
    ZError() = default;
private:
    string x_;
};
class ZResp {
public:
    ZResp() = default;
    ZResp(int i, string s)
        :i_(i), s_(s)
    {}
private:
    int i_;
    string s_;
};
template<typename R>
class Outcome {
public:
    Outcome(const ZError& e)
        :error_(e)
    {cout << "const error" << endl;}
    Outcome(ZError&& e)
        :error_(move(e))
    {cout << "r error" << endl;}
    template<typename ...Args, typename = enable_if_t<is_constructible_v<R, Args&&...>>>
    Outcome(Args&&... args)
        :result_(forward<Args>(args)...)
    {cout << "forward Args" << endl;}
    const ZError& error() const {return error_;}
    ZError& error() {return error_;}
private:
    R result_;
    ZError error_;
};

void test_type_trait() {
    cout << "Test type trait ..." << endl;
    Outcome<ZResp> outcome{ZError()};
    //Outcome<ZResp> outcome(ZError()); // for member ‘error’ in ‘outcome’, which is of non-class type ‘Outcome<ZResp>(ZError (*)())
    Outcome<ZResp> outcome2(1, "");

    Outcome<ZResp> outcome3(outcome.error());
}

void test_false_sharing() {
    cout  << "Test false sharing ..." << endl;
    struct foo {
        int32_t x;
        int32_t y;
    };
    struct foo64 {
        int64_t x;
        int64_t y;
    };

    foo f32;
    foo64 f64;

    auto start = chrono::steady_clock::now();
    thread t{[&f32]() -> int32_t {
        int s = 0;
        for (auto i = 0; i < 1000000; ++i) {
            s += f32.x;
        }
        return s;
    }};
    thread t2{[&f32]() {
        for (auto i = 0; i < 1000000; ++i) {
            ++f32.y;
        }
    }};
    t.join();
    t2.join();
    auto end = chrono::steady_clock::now();

    thread t3{[&f64]() -> int64_t {
        int s = 0;
        for (auto i = 0; i < 1000000; ++i) {
            s += f64.x;
        }
        return s;
    }};
    thread t4{[&f64]() {
        for (auto i = 0; i < 1000000; ++i) {
            ++f64.y;
        }
    }};
    t3.join();
    t4.join();
    auto end64 = chrono::steady_clock::now();

    thread t5{[&f32]() {
        for (auto i = 0; i < 1000000; ++i) {
            ++f32.y;
        }
    }};
    t5.join();
    auto end_single_32 = chrono::steady_clock::now();
    cout << "32bit elapsed " << (end - start).count() << endl
        << "64bit elapsed " << (end64 - end).count() << endl
        << "32bit single elapsed " << (end_single_32 - end64).count() << endl;
}

void test_async() {
    cout << "Test async ..." << endl;
}

void test_align() {
    cout << "Test align ..." << endl;
    struct A {
        char a;
        char b;
    };
    struct B {
        char a;
        int b;
    };
    struct C {
        char a;
    };
    struct __attribute__((packed)) D {
        char a;
        int b;
    };
    static_assert(sizeof(A) == 2);
    static_assert(sizeof(B) == 8);
    static_assert(sizeof(C) == 1);
    static_assert(sizeof(D) == 5);
}

void test_m_cout() {
    cout << "Test multi thread std out ..." << endl;
}

void test_any() {
    cout << "Test any ..." << endl;

    std::any a1 = 1;
    auto a2 = make_any<int>(2);
    auto a3 = any_cast<int>(a2);
}

void test_spin_lock() {
    cout << "Test spin lock ..." << endl;
    SpinLock lock;

    string critical;
    thread t1{[&]() {
        lock.Lock();
        critical = "critical_t1";
        lock.Unlock();
    }};
    lock.Lock();
    critical = "qwerty";
    lock.Unlock();
    t1.join();
    cout << critical << endl;
}

// generate leetcode array test input
void generate_random_array(int range, int len) {
    mt19937 rd{random_device{}()};
    uniform_int_distribution<int> dist(0, range);
    stringstream ss;
    ss << "[";
    for (int i = 0; i < len - 1; ++i) {
        ss << to_string(dist(rd)) << ",";
    }
    ss << to_string(dist(rd)) << "]";
    cout << ss.str() << endl;
}

void test_iterator() {
    cout << "Test iterator ..." << endl;
    vector<int> v{1,2,3,4,5,6,7,8};
    assert(v.end() - v.begin() == v.size());
    for (auto ri = v.rbegin(); ri != v.rend(); ++ri) {
        cout << *ri;
    }
    cout << endl;
}

void test_double_accumulate() {
    cout << "Test double accum ..." << endl;
    vector<double> v{1.1,2.2,3.3,4.4,5.5,6.6,7.7};
    auto r = accumulate(v.begin(), v.end(), 0.0);
    auto r2 = accumulate(v.begin(), v.end(), 0);
    cout << r << " " << r2 << endl;

    auto i2 = new int(1), i = new int(2);
    cout << typeid(i).name() << endl << typeid(i2).name() << endl;
}

void main_test() {
    test_make_request();
    //test_response();
    test_future();
    test_stream();
    test_type_trait();
    //test_false_sharing();
    test_async();
    //test_reorder();
    // TODO: run in android
    //test_seq_cst();
    //test_acq_rel();
    test_align();
    test_m_cout();
    test_any();
    //test_epoll();
    //generate_random_array(900, 500);
    test_spin_lock();
    test_iterator();
    test_double_accumulate();
    template_test();
}

int main()
{
    main_test();
}
