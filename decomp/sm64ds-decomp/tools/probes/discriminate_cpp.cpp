/* C++ differential probe. The game is C++ (CodeWarrior), and point-releases are
 * likelier to diverge on C++ codegen (vtables, ct/dtors, member calls, new/delete,
 * templates) than on plain C. Compiled with each candidate version; probe_versions.py
 * flags any function whose bytes differ. (.cpp extension -> C++; no -lang c99.) */

struct Base {
    int x;
    virtual int  f();
    virtual void g(int);
    virtual ~Base();
};
struct Derived : Base {
    int y;
    int  f();
    void g(int);
    ~Derived();
};

int  Base::f()        { return x; }
void Base::g(int v)   { x = v; }
Base::~Base()         {}
int  Derived::f()     { return x + y; }
void Derived::g(int v){ y = v; x = v + 1; }
Derived::~Derived()   {}

int  call_virt(Base *b)        { return b->f(); }
void call_virt2(Base *b, int v){ b->g(v); }

extern "C" void *operator_new_stub(unsigned);
Base *make_one()        { return new Derived(); }
void  kill_one(Base *b) { delete b; }

struct Vec3 { int x, y, z; };
Vec3 *vec_set(Vec3 *v, int a, int b, int c) { v->x = a; v->y = b; v->z = c; return v; }

template <class T> T tmax(T a, T b) { return a > b ? a : b; }
int   use_tmax_i(int a, int b)     { return tmax<int>(a, b); }
float use_tmax_f(float a, float b) { return tmax<float>(a, b); }

int sum_methods(Derived *d, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += d->f();
    return s;
}
