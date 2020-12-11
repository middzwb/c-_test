#include <iostream>
#include <memory>
#include <sstream>

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

void main_test() {
    {
        auto ss{make_shared<istringstream>("test sstringstream")};
        auto ret = make_request(ss);
        if (ret) {
            cout << ret->str() << endl;
        }
        auto ret2 = make_request(nullptr);
        if (ret2) {
            cout << ret2->str() << endl;
        }
        else {
            cout << "no out stringstream" << endl;
        }
    }

}

int main()
{
    main_test();
}
