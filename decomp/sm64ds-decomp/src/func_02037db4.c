/* func_02037db4 is a compiler-generated this-adjusting virtual-destructor
 * thunk (CodeWarrior _ZThn56_...D1Ev form): ldr ip,[pc,#4]; add r0,r0,ip; b D1
 * It adjusts `this` by -56 (size of the primary base subobject) then tail-branches
 * to the complete object destructor (SphereClsn::~SphereClsn @0x2037cb0).
 * Cannot be expressed as plain C; the byte-identical thunk is emitted by compiling
 * the C++ multiple-inheritance class below (secondary base at offset 56).
 * Matched symbol: _ZThn56_N7DerivedD1Ev (== func_02037db4).
 *
 * compile as C++:  mwccarm -lang=c++ -O4,p -enum int -char signed -interworking -proc arm946e -gccext,on
 */
struct Base1 {
    int a0; int a1; int a2; int a3; int a4; int a5;
    int a6; int a7; int a8; int a9; int a10; int a11; int a12; /* vtable(4)+52 = 56 */
    virtual ~Base1(); virtual void f1();
};
struct Base2 { int b; virtual ~Base2(); virtual void g1(); };
struct Derived : Base1, Base2 { virtual ~Derived(); };
Derived::~Derived() {}
