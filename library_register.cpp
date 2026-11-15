#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct ReturnRecord {
    int id = 0;
    std::string memberName;
    std::string bookTitle;
    std::string dueDate;
    std::string returnDate;

    int daysLate() const;
    long fineUGX() const;
};

bool parseDate(const std::string& s, int& y, int& m, int& d) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    int idx[] = {0, 1, 2, 3, 5, 6, 8, 9};
    for (int i : idx) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    y = std::stoi(s.substr(0, 4));
    m = std::stoi(s.substr(5, 2));
    d = std::stoi(s.substr(8, 2));
    if (y < 1900 || y > 2200 || m < 1 || m > 12 || d < 1) return false;
    int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    int maxd = dim[m - 1] + ((m == 2 && leap) ? 1 : 0);
    return d <= maxd;
}

bool isValidDate(const std::string& s) {
    int y, m, d;
    return parseDate(s, y, m, d);
}

long dayNumber(const std::string& s) {
    int y, m, d;
    if (!parseDate(s, y, m, d)) return -1;
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    long yoe = y - era * 400;
    long doy = (153L * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

int ReturnRecord::daysLate() const {
    long due = dayNumber(dueDate);
    long ret = dayNumber(returnDate);
    if (due < 0 || ret < 0 || ret <= due) return 0;
    return static_cast<int>(ret - due);
}

long ReturnRecord::fineUGX() const {
    return 500L * daysLate();
}

bool cleanText(const std::string& s) {
    if (s.empty()) return false;
    bool hasChar = false;
    for (char c : s) {
        if (c == '|' || c == '\n' || c == '\r') return false;
        if (c != ' ' && c != '\t') hasChar = true;
    }
    return hasChar;
}

bool isValidRecord(const ReturnRecord& r) {
    return r.id > 0 && cleanText(r.memberName) && cleanText(r.bookTitle)
        && isValidDate(r.dueDate) && isValidDate(r.returnDate);
}

template <typename T>
class ReturnsRegister {
private:
    struct Node {
        T data;
        Node* prev;
        Node* next;
        Node(const T& r) : data(r), prev(nullptr), next(nullptr) {}
    };

    Node* head_;
    Node* tail_;
    std::size_t count_;

    Node* findNode(int id) const {
        for (Node* p = head_; p != nullptr; p = p->next) {
            if (p->data.id == id) return p;
        }
        return nullptr;
    }

    void unlink(Node* n) {
        if (n->prev) n->prev->next = n->next;
        else head_ = n->next;
        if (n->next) n->next->prev = n->prev;
        else tail_ = n->prev;
        delete n;
        --count_;
    }

public:
    ReturnsRegister() : head_(nullptr), tail_(nullptr), count_(0) {}

    ReturnsRegister(const ReturnsRegister& other) : head_(nullptr), tail_(nullptr), count_(0) {
        for (Node* p = other.head_; p != nullptr; p = p->next) {
            pushBack(p->data);
        }
    }

    ReturnsRegister& operator=(const ReturnsRegister& other) {
        if (this != &other) {
            clear();
            for (Node* p = other.head_; p != nullptr; p = p->next) {
                pushBack(p->data);
            }
        }
        return *this;
    }

    ~ReturnsRegister() {
        clear();
    }

    void clear() {
        Node* p = head_;
        while (p != nullptr) {
            Node* nx = p->next;
            delete p;
            p = nx;
        }
        head_ = nullptr;
        tail_ = nullptr;
        count_ = 0;
    }

    bool pushBack(const T& r) {
        if (!isValidRecord(r) || findNode(r.id) != nullptr) return false;
        Node* n = new Node(r);
        n->prev = tail_;
        if (tail_) tail_->next = n;
        else head_ = n;
        tail_ = n;
        ++count_;
        return true;
    }

    bool pushFront(const T& r) {
        if (!isValidRecord(r) || findNode(r.id) != nullptr) return false;
        Node* n = new Node(r);
        n->next = head_;
        if (head_) head_->prev = n;
        else tail_ = n;
        head_ = n;
        ++count_;
        return true;
    }

    bool removeFirst(T* out = nullptr) {
        if (!head_) return false;
        if (out) *out = head_->data;
        unlink(head_);
        return true;
    }

    bool removeLast(T* out = nullptr) {
        if (!tail_) return false;
        if (out) *out = tail_->data;
        unlink(tail_);
        return true;
    }

    bool removeById(int id, T* out = nullptr) {
        Node* n = findNode(id);
        if (!n) return false;
        if (out) *out = n->data;
        unlink(n);
        return true;
    }

    const T* findById(int id) const {
        Node* n = findNode(id);
        return n ? &n->data : nullptr;
    }

    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }

    std::vector<T> toVector() const {
        std::vector<T> v;
        v.reserve(count_);
        for (Node* p = head_; p != nullptr; p = p->next) {
            v.push_back(p->data);
        }
        return v;
    }

    bool replaceWith(const std::vector<T>& v) {
        ReturnsRegister<T> temp;
        for (const auto& r : v) {
            if (!temp.pushBack(r)) return false;
        }
        clear();
        head_ = temp.head_;
        tail_ = temp.tail_;
        count_ = temp.count_;
        temp.head_ = nullptr;
        temp.tail_ = nullptr;
        temp.count_ = 0;
        return true;
    }

    bool selfCheck() const {
        std::size_t n = 0;
        Node* last = nullptr;
        for (Node* p = head_; p != nullptr; p = p->next) {
            if (p->prev != last) return false;
            last = p;
            ++n;
            if (n > count_) return false;
        }
        if (last != tail_ || n != count_) return false;
        if (!head_ && (tail_ || count_)) return false;
        if (head_ && head_->prev) return false;
        if (tail_ && tail_->next) return false;
        return true;
    }

    void printForward(std::ostream& os) const {
        if (!head_) {
            os << "(register is empty)\n";
            return;
        }
        os << std::left << std::setw(6) << "ID" << std::setw(20) << "Member"
           << std::setw(30) << "Book" << std::setw(12) << "Due"
           << std::setw(12) << "Returned" << std::right << std::setw(6) << "Late"
           << std::setw(10) << "Fine(UGX)\n";
        for (Node* p = head_; p != nullptr; p = p->next) {
            const auto& r = p->data;
            os << std::left << std::setw(6) << r.id << std::setw(20) << r.memberName.substr(0, 19)
               << std::setw(30) << r.bookTitle.substr(0, 29) << std::setw(12) << r.dueDate
               << std::setw(12) << r.returnDate << std::right << std::setw(6) << r.daysLate()
               << std::setw(10) << r.fineUGX() << "\n";
        }
    }

    void printBackward(std::ostream& os) const {
        if (!tail_) {
            os << "(register is empty)\n";
            return;
        }
        os << std::left << std::setw(6) << "ID" << std::setw(20) << "Member"
           << std::setw(30) << "Book" << std::setw(12) << "Due"
           << std::setw(12) << "Returned" << std::right << std::setw(6) << "Late"
           << std::setw(10) << "Fine(UGX)\n";
        for (Node* p = tail_; p != nullptr; p = p->prev) {
            const auto& r = p->data;
            os << std::left << std::setw(6) << r.id << std::setw(20) << r.memberName.substr(0, 19)
               << std::setw(30) << r.bookTitle.substr(0, 29) << std::setw(12) << r.dueDate
               << std::setw(12) << r.returnDate << std::right << std::setw(6) << r.daysLate()
               << std::setw(10) << r.fineUGX() << "\n";
        }
    }

    int load(const std::string& path, int* skipped = nullptr) {
        std::ifstream in(path);
        if (!in) return -1;
        ReturnsRegister<T> temp;
        int bad = 0;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> f;
            std::string cur;
            std::istringstream ss(line);
            while (std::getline(ss, cur, '|')) f.push_back(cur);
            if (!line.empty() && line.back() == '|') f.push_back("");
            if (f.size() != 5) {
                ++bad;
                continue;
            }
            char* end = nullptr;
            long id = std::strtol(f[0].c_str(), &end, 10);
            if (f[0].empty() || *end != '\0' || id <= 0 || id > 2000000000L) {
                ++bad;
                continue;
            }
            T r{static_cast<int>(id), f[1], f[2], f[3], f[4]};
            if (!temp.pushBack(r)) ++bad;
        }
        if (skipped) *skipped = bad;
        int n = static_cast<int>(temp.count_);
        clear();
        head_ = temp.head_;
        tail_ = temp.tail_;
        count_ = temp.count_;
        temp.head_ = nullptr;
        temp.tail_ = nullptr;
        temp.count_ = 0;
        return n;
    }

    bool save(const std::string& path) const {
        std::ofstream out(path, std::ios::trunc);
        if (!out) return false;
        out << "# id|member|title|due|returned\n";
        for (Node* p = head_; p != nullptr; p = p->next) {
            out << p->data.id << '|' << p->data.memberName << '|' << p->data.bookTitle << '|'
                << p->data.dueDate << '|' << p->data.returnDate << '\n';
        }
        out.flush();
        return static_cast<bool>(out);
    }
};

const char* SAMPLE_DATA = R"DATA(# Library returns register sample data
# id|member|title|due|returned
1001|Achieng Grace|Things Fall Apart|2026-10-20|2026-10-22
